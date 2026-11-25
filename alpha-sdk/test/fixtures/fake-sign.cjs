// Stands in for signtool in tests: adds an Authenticode-shaped signature to a PE file
// (8-byte alignment padding, a certificate table at the end, the security directory
// pointing at it, a new checksum). `--tamper` also changes one byte of the code.
const fs = require("node:fs");
const [file, flag] = process.argv.slice(2);
let data = fs.readFileSync(file);
const opt = data.readUInt32LE(0x3c) + 24;
const dirs = opt + (data.readUInt16LE(opt) === 0x20b ? 112 : 96);
const pad = Buffer.alloc((8 - (data.length % 8)) % 8);
const cert = Buffer.concat([Buffer.from([16, 0, 0, 0, 0, 2, 2, 0]), Buffer.from("FAKE-PKCS7")]);
const offset = data.length + pad.length;
data = Buffer.concat([data, pad, cert]);
data.writeUInt32LE(offset, dirs + 32);
data.writeUInt32LE(cert.length, dirs + 36);
data.writeUInt32LE(0x1234567, opt + 64);
if (flag === "--tamper") data[0x400] ^= 0xff;
fs.writeFileSync(file, data);
