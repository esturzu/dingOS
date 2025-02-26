# creates a file filled with 4-byte hex representting its address
# verify with `xxd -g 4 test.dd > test.hex`
size_mb = 1
size_bytes = size_mb * 1024 * 1024


with open("test.dd", "wb") as f:
    for i in range(3, size_bytes, 4):
        f.write((i % (0x100000000)).to_bytes(4, "little"))
