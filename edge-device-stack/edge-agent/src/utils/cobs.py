"""Consistent Overhead Byte Stuffing implementation."""
from __future__ import annotations


def encode(data: bytes) -> bytes:
    if not data:
        return b"\x01\x00"
    output = bytearray()
    idx = 0
    length = len(data)
    while idx < length:
        block_start = idx
        idx += 1
        while idx - block_start < 0xFF and idx <= length and (idx == length or data[idx - 1] != 0):
            idx += 1
        code = idx - block_start
        output.append(code)
        output.extend(data[block_start : block_start + code - 1])
        if idx <= length and data[idx - 1] == 0:
            idx += 1
    output.append(0)
    return bytes(output)


def decode(data: bytes) -> bytes:
    if not data or data[-1] != 0:
        raise ValueError("COBS frame missing terminator")
    output = bytearray()
    idx = 0
    length = len(data) - 1
    while idx < length:
        code = data[idx]
        if code == 0:
            raise ValueError("Invalid COBS code 0")
        idx += 1
        block_end = idx + code - 1
        if block_end > length:
            raise ValueError("COBS block overruns buffer")
        output.extend(data[idx:block_end])
        idx = block_end
        if code < 0xFF and idx < length:
            output.append(0)
    return bytes(output)
