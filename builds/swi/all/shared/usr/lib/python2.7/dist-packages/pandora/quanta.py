import os.path, struct, time

_board_info = None

def board_info(field):
    global _board_info
    if _board_info:
        return _board_info.get(field)

    info = {}

    for e in ["/sys/devices/soc.0/ffe03000.i2c/i2c-0/i2c-2/2-0054/eeprom",
              "/sys/devices/e0000000.soc8541/e0003000.i2c/i2c-0/0-0054/eeprom",
              "/sys/devices/e0000000.soc8541/e0003000.i2c/i2c-0/0-0053/eeprom"]:
        try:
            f = file(e)
            if f.read(3) == "\xff\x01\xe0":
                f = file(e)
                break
        except IOError:
            pass
    else:
        return

    fields = {
        0xff: ("magic_num", "!B"),
        0x01: ("product_name", None),
        0x02: ("part_num", None),
        0x03: ("serial_num", None),
        0x04: ("mac_addr", "MAC"),
        0x05: ("mfg_date", "DATE"),
        0x06: ("card_type", "!L"),
        0x07: ("hw_version", "!L"),
        0x08: ("label_version", None),
        0x09: ("model_name", None),
        0x0a: ("sw_version", "!L"),
        0x00: ("crc16", "!H"),
        }

    while True:
        t = f.read(1)
        if not t:
            break
        t = ord(t)
        l = f.read(1)
        if not l:
            break
        l = ord(l)
        if l < 1:
            break
        x = f.read(l)
        if len(x) != l:
            break
        if t in fields:
            try:
                t, c = fields[t]
                if c == "MAC":
                    x = "%02x:%02x:%02x:%02x:%02x:%02x" % struct.unpack("!6B", x)
                if c == "DATE":
                    x = time.gmtime(time.mktime(struct.unpack("!HBB", x)
                                                + (0, 0, 0, 0, 0, 0)))
                elif c:
                    x = struct.unpack(c, x)[0]
            except struct.error:
                pass
        info[t] = x

    _board_info = info
    return _board_info.get(field)
