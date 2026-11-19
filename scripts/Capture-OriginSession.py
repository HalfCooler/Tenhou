"""Record and trim redacted messages from the repository's original Windows client."""

import argparse
import hashlib
import json
import sys
from datetime import datetime
from pathlib import Path
from threading import Event, Lock


ROOT = Path(__file__).resolve().parents[1]
HOOK = Path(__file__).with_name("origin-session-hook.js")
ORIGIN_SHA256 = "96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6"
FORMAT = "tenhou-origin-session/1"
COMMANDS = """\
  mark [@序号] 备注         给指定事件打标。省略序号时标记当前最后一条事件
  show [起始序号 结束序号]    查看闭区间。省略区间时查看最新 20 条
  export 起始 结束 文件     导出闭区间及对应标记为新 JSONL，路径可含空格
  help / quit              显示命令 / 退出
"""
LIVE_COMMANDS = """\
  attach [PID]            附加到原版。省略 PID 时选择唯一运行的 tenhou.exe
  detach                  解除附加
  start / pause           开始 / 暂停记录消息
"""


def timestamp(tick=None):
    instant = datetime.now() if tick is None else datetime.fromtimestamp(tick / 1000)
    return instant.astimezone().isoformat(timespec="milliseconds")


def write_line(output, record):
    print(json.dumps(record, ensure_ascii=False, separators=(",", ":")), file=output)


class Journal:
    def __init__(self, path, review=False):
        self.path = path.resolve()
        self.review = review
        self.lock = Lock()
        if review:
            with self.path.open(encoding="utf-8") as source:
                self.header = json.loads(source.readline())
                if not isinstance(self.header, dict) or self.header.get("format") != FORMAT:
                    raise ValueError("不是原版采集记录")
                self.records = [json.loads(line) for line in source if line.strip()]
        else:
            self.header = {
                "format": FORMAT, "origin_version": "130/0005", "origin_sha256": ORIGIN_SHA256,
                "created_at": timestamp(), "outbound": "Winsock send", "inbound": "0x415F48 / WM 0x8209",
            }
            self.records = []
            self.path.parent.mkdir(parents=True, exist_ok=True)
            with self.path.open("x", encoding="utf-8") as output:
                write_line(output, self.header)

    def add(self, kind, *, tick=None, **fields):
        with self.lock:
            sequence = self.records[-1]["seq"] + 1 if self.records else 1
            record = {"seq": sequence, "at": timestamp(tick), "kind": kind, **fields}
            if not self.review:
                with self.path.open("a", encoding="utf-8") as output:
                    write_line(output, record)
            self.records.append(record)
            return sequence

    def mark(self, text, anchor=None):
        with self.lock:
            if anchor is None and self.records:
                anchor = self.records[-1].get("anchor", self.records[-1]["seq"])
            if not any(record["seq"] == anchor and record["kind"] != "mark" for record in self.records):
                raise ValueError("请指定记录内的一条事件作为标记位置")
        return self.add("mark", anchor=anchor, note=text)

    def select(self, first, last):
        with self.lock:
            selected = [row for row in self.records if first <= row.get("anchor", row["seq"]) <= last]
        if not selected:
            raise ValueError("所选区间内没有事件")
        return selected

    def latest(self):
        with self.lock:
            return self.records[-20:]

    def export(self, first, last, destination):
        rows = self.select(first, last)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open("x", encoding="utf-8") as output:
            write_line(output, {**self.header, "range": [first, last], "exported_at": timestamp()})
            for row in rows:
                write_line(output, row)
        return len(rows)


class Capture:
    def __init__(self, frida, journal):
        self.device = frida.get_local_device()
        self.journal = journal
        self.session = None
        self.script = None
        self.recording = False

    def attach(self, pid=None):
        if self.session and not self.session.is_detached:
            raise ValueError("当前原版进程仍在运行。可用 detach 解除后再附加")
        if pid is None:
            clients = [p for p in self.device.enumerate_processes() if p.name.casefold() == "tenhou.exe"]
            if len(clients) != 1:
                pids = ", ".join(str(p.pid) for p in clients)
                raise ValueError(f"请启动原版并停在登录页。多个客户端时使用 attach PID（当前候选 {pids or '无'}）")
            pid = clients[0].pid
        processes = self.device.enumerate_processes(pids=[pid], scope="metadata")
        if not processes:
            raise ValueError(f"PID {pid} 已退出或不存在")
        image = processes[0].parameters.get("path")
        if not image or Path(image).name.casefold() != "tenhou.exe":
            raise ValueError("指定进程不是 tenhou.exe 或无法读取进程镜像路径")
        with Path(image).open("rb") as binary:
            if hashlib.file_digest(binary, "sha256").hexdigest() != ORIGIN_SHA256:
                raise ValueError("原版 EXE 的 SHA-256 与仓库取证版本不符")

        session = self.device.attach(pid)
        detached = Event()
        stream = self.journal.add("attach", pid=pid)

        def on_detach(reason, _crash):
            detached.set()
            self.journal.add("detach", stream=stream, reason=reason)

        def on_message(message, _data):
            if message["type"] != "send":
                self.journal.add("gap", stream=stream, reason="hook-error")
                return
            payload = message["payload"]
            self.journal.add(stream=stream, **payload)

        session.on("detached", on_detach)
        try:
            script = session.create_script(HOOK.read_text(encoding="utf-8"))
            script.on("message", on_message)
            script.load()
            script.exports_sync.set_recording(False)
        except Exception:
            if not session.is_detached:
                session.detach()
            raise
        self.session, self.script, self.stream, self.detached = session, script, stream, detached
        self.recording = False
        print(f"已附加 PID {pid}，输入 start 开始记录")

    def record(self, enabled):
        if not self.session or self.session.is_detached or self.detached.is_set():
            raise ValueError("请先 attach 原版进程")
        if self.recording != enabled:
            if enabled:
                self.journal.add("start", stream=self.stream)
            self.script.exports_sync.set_recording(enabled)
            if not enabled:
                self.journal.add("pause", stream=self.stream)
            self.recording = enabled
        print("正在记录" if enabled else "已暂停")

    def close(self):
        if self.session and not self.session.is_detached:
            self.session.detach()
        self.session = None


def sequence_range(first, last):
    first, last = int(first), int(last)
    if not 1 <= first <= last:
        raise ValueError("请输入有效区间，起始序号须小于或等于结束序号")
    return first, last


def display(row):
    if row["kind"] == "frame":
        body = " ".join(f"{k}={json.dumps(v, ensure_ascii=False)}" for k, v in row["attrs"].items())
        message = f'{row["direction"]} {row["tag"]} {body}'
        if "send" in row:
            message += f' [send {row["send"]["accepted"]}/{row["send"]["requested"]}]'
    elif row["kind"] == "mark":
        message = f'标记 @{row["anchor"]} {row["note"]}'
    else:
        message = " ".join(str(row[key]) for key in ("kind", "pid", "reason") if key in row)
    print(f'#{row["seq"]} {row["at"]} {message}'.rstrip())


def prompt(journal, capture, errors):
    print("输入 help 查看命令。" if capture else "审阅模式，标记随 export 写入新文件。输入 help 查看命令。")
    while True:
        try:
            command, _, argument = input("origin> ").strip().partition(" ")
            argument = argument.strip()
            if command == "quit":
                break
            if command == "help":
                print((LIVE_COMMANDS if capture else "") + COMMANDS, end="")
            elif command == "attach" and capture:
                capture.attach(int(argument) if argument else None)
            elif command == "detach" and capture:
                capture.close()
                print("已解除附加")
            elif command in ("start", "pause") and capture:
                capture.record(command == "start")
            elif command == "mark":
                anchor = None
                if argument.startswith("@"):
                    position, _, argument = argument.partition(" ")
                    anchor = int(position[1:])
                if not argument.strip():
                    raise ValueError("需要填写备注")
                print(f"已标记 #{journal.mark(argument.strip(), anchor)}")
            elif command == "show":
                if argument:
                    first, last = argument.split()
                    rows = journal.select(*sequence_range(first, last))
                else:
                    rows = journal.latest()
                for row in rows:
                    display(row)
            elif command == "export":
                first, last, path = argument.split(maxsplit=2)
                destination = Path(path.strip('"'))
                count = journal.export(*sequence_range(first, last), destination)
                print(f"已导出 {count} 条至 {destination.resolve()}")
            elif command:
                print("此模式下无该命令。输入 help 查看用法。")
        except (EOFError, KeyboardInterrupt):
            print()
            break
        except errors as error:
            print(error, file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_subparsers(dest="mode", required=True)
    live = modes.add_parser("capture", help="attach to the original client and record")
    live.add_argument("--pid", type=int, help="original tenhou.exe process ID")
    live.add_argument("--out", type=Path, help="new JSONL output path")
    review = modes.add_parser("review", help="view, annotate and export a captured interval")
    review.add_argument("path", type=Path)
    args = parser.parse_args()
    errors = (OSError, ValueError)
    if args.mode == "capture":
        try:
            import frida
        except ImportError:
            parser.error("实时采集需要 Frida，请运行 python -m pip install frida==17.27.1")
        errors += (frida.ProcessNotFoundError, frida.PermissionDeniedError, frida.ProcessNotRespondingError,
                   frida.NotSupportedError, frida.InvalidArgumentError, frida.InvalidOperationError,
                   frida.TransportError, frida.TimedOutError, frida.core.RPCException)

    try:
        if args.mode == "review":
            prompt(Journal(args.path, review=True), None, errors)
        else:
            path = args.out or ROOT / "build" / "origin-capture" / datetime.now().strftime("session-%Y%m%d-%H%M%S-%f.jsonl")
            journal = Journal(path)
            capture = Capture(frida, journal)
            print(f"记录文件 {journal.path}")
            try:
                try:
                    capture.attach(args.pid)
                except errors as error:
                    print(error, file=sys.stderr)
                prompt(journal, capture, errors)
            finally:
                capture.close()
    except errors as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
