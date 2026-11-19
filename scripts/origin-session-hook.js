'use strict';

const maxFrame = 65536;
const integer = /^-?\d{1,12}$/;
const numbers = /^(?:-?\d+(?:\.\d+)?)(?:,(?:-?\d+(?:\.\d+)?)?)*$/;
const room = /^(?:\d{1,4}|L\d{4}|C\d{8})$/;
const tileTag = /^[TUVWDEFGdefg]\d{0,3}$/;
const numericFields = {
    HELO: 'PF4 PF3 expiredays ratingscale opt',
    RANK: 'v2', PXR: 'V v', CS: 'players premiumonly joinfee rule csrule join',
    CSREG: 'code', JOIN: 't type', REJOIN: 't', ERR: 'code',
    GO: 'type', TAIKYOKU: 'oya', SAIKAI: 'oya t', UN: 'dan rate',
    INIT: 'seed ten oya hai hai0 hai1 hai2 hai3',
    REINIT: 'seed ten oya hai hai0 hai1 hai2 hai3 kawa0 kawa1 kawa2 kawa3 m0 m1 m2 m3',
    D: 'p hai t', N: 'who m type hai t', DORA: 'hai', REACH: 'who step ten',
    AGARI: 'ba hai m machi ten yaku yakuman doraHai doraHaiUra who fromWho paoWho sc owari',
    RYUUKYOKU: 'ba hai0 hai1 hai2 hai3 ten sc owari',
    PROF: 'dan rate', DATE: 'date',
};
const presenceFields = {
    HELO: 'name uname auth expire', AUTH: 'val', CHAT: 'uname',
    CS: 'title', CSREG: 'uname', RANKING: 'rankingcs',
    UN: 'n0 n1 n2 n3', TAIKYOKU: 'log', SAIKAI: 'log',
    SHUFFLE: 'seed', ERR: 'text message',
};
const fields = entries => Object.fromEntries(Object.entries(entries).map(([key, value]) => [key, new Set(value.split(' '))]));
const numeric = fields(numericFields), presence = fields(presenceFields);
const tags = new Set([
    ...Object.keys(numeric), ...Object.keys(presence),
    'LN', 'Z', 'GOK', 'NEXTREADY', 'BYE', 'INVALID', 'LNERR', 'DISCONNECT',
]);
const present = value => value === '' ? '<empty>' : '<present>';

function attribute(tag, key, value) {
    if (tag === 'HELO' && (key === 'name' || key === 'uname') && value === 'NoName') return value;
    if (presence[tag]?.has(key)) return present(value);
    if (numeric[tag]?.has(key)) return value.length <= 4096 && numbers.test(value) ? value : present(value);
    if (key === 'lobby' && /^(CHAT|CS|CSREG|GO)$/.test(tag)) return room.test(value) ? value : present(value);
    if (tag === 'CHAT' && key === 'text') return /^%2Flobby%20\d{1,4}$/i.test(value) ? value : undefined;
    if (tag === 'LN' && /^(n|j|g)$/.test(key)) {
        return value.length <= 4096 && /^[A-Za-z\d,-]+$/.test(value) ? value : present(value);
    }
    if (tag === 'HELO' && /^(tid|ver|version|sx|closing)$/.test(key)) {
        return /^(w0|[MF]|\d{1,5}(?:\/\d{1,5})?)$/.test(value) ? value : present(value);
    }
    if (tileTag.test(tag) && key === 't' && integer.test(value)) return value;
    if (tag === 'RYUUKYOKU' && key === 'type') {
        return /^(yao9|kaze4|reach4|ron3|kan4|nm)$/.test(value) ? value : present(value);
    }
}

function redact(frame) {
    const header = /^<([A-Za-z][A-Za-z0-9_]{0,31})(?=\s|\/?>)/.exec(frame);
    if (!header) return {tag: 'UNKNOWN', attrs: {}};
    const tag = header[1];
    if (!tags.has(tag) && !tileTag.test(tag)) return {tag: 'UNKNOWN', attrs: {}};
    const attrs = {};
    const token = /\s+([A-Za-z_][A-Za-z0-9_]*)(?:\s*=\s*(?:"([^"]*)"|'([^']*)'))?/y;
    token.lastIndex = header[0].length;
    for (let match; (match = token.exec(frame));) {
        const [, key, doubleQuoted, singleQuoted] = match;
        const value = doubleQuoted ?? singleQuoted;
        if (value === undefined) {
            if (tag === 'CSREG' && key === 'FAILED') attrs.FAILED = true;
            continue;
        }
        const safe = attribute(tag, key, value);
        if (safe !== undefined) attrs[key] = safe;
    }
    return {tag, attrs};
}

function readFrame(buffer, length) {
    // Interpret only ASCII syntax. High bytes in display text must not prevent redaction.
    const bytes = new Uint8Array(buffer.readByteArray(length));
    let frame = '';
    for (let offset = 0; offset < bytes.length; offset += 4096) {
        frame += String.fromCharCode(...bytes.subarray(offset, offset + 4096));
    }
    return redact(frame);
}

const base = Process.mainModule.base;
const gameSocket = base.add(0xf4e3c);
const dispatchReturn = base.add(0x15f4e);
let recording = false;

function observe(direction, buffer, length) {
    const tick = Date.now();
    if (length < 1 || length > maxFrame) return {kind: 'gap', direction, tick, reason: 'frame-size'};
    try {
        return {kind: 'frame', direction, tick, ...readFrame(buffer, length)};
    } catch (_) {
        return {kind: 'gap', direction, tick, reason: 'frame-read'};
    }
}

Interceptor.attach(Process.getModuleByName('ws2_32.dll').getExportByName('send'), {
    onEnter(args) {
        const socket = args[0].toUInt32();
        if (!recording || socket === 0xffffffff || socket !== gameSocket.readU32()) return;
        this.length = args[2].toInt32();
        this.record = observe('out', args[1], this.length - 1); // The original includes the NUL in send's length.
    },
    onLeave(result) {
        if (!this.record) return;
        const sent = result.toInt32();
        if (sent !== this.length) this.record.send = {requested: this.length, accepted: sent};
        send(this.record);
    },
});

Interceptor.attach(Process.getModuleByName('user32.dll').getExportByName('SendMessageA'), {
    onEnter(args) {
        if (!recording || !this.returnAddress.equals(dispatchReturn) || args[1].toUInt32() !== 0x8209) return;
        // 0x415F52 uses ESI as the original NUL-delimited frame length.
        send(observe('in', args[3], this.context.esi.toUInt32()));
    },
});

rpc.exports.setRecording = enabled => { recording = enabled; };
