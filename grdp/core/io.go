package core

import (
	"encoding/binary"
	"io"
)

type ReadBytesComplete func(result []byte, err error)

func StartReadBytes(len int, r io.Reader, cb ReadBytesComplete) {
	b := make([]byte, len)
	go func() {
		_, err := io.ReadFull(r, b)
		//glog.Debug("StartReadBytes Get", n, "Bytes:", hex.EncodeToString(b))
		cb(b, err)
	}()
}

func ReadBytes(len int, r io.Reader) ([]byte, error) {
	b := make([]byte, len)
	length, err := io.ReadFull(r, b)
	return b[:length], err
}

func ReadByte(r io.Reader) (byte, error) {
	b, err := ReadBytes(1, r)
	return b[0], err
}

func ReadUInt8(r io.Reader) (uint8, error) {
	b, err := ReadBytes(1, r)
	return uint8(b[0]), err
}

func ReadUint16LE(r io.Reader) (uint16, error) {
	b := make([]byte, 2)
	_, err := io.ReadFull(r, b)
	if err != nil {
		return 0, nil
	}
	return binary.LittleEndian.Uint16(b), nil
}

func ReadUint16BE(r io.Reader) (uint16, error) {
	b := make([]byte, 2)
	_, err := io.ReadFull(r, b)
	if err != nil {
		return 0, nil
	}
	return binary.BigEndian.Uint16(b), nil
}

func ReadUInt32LE(r io.Reader) (uint32, error) {
	b := make([]byte, 4)
	_, err := io.ReadFull(r, b)
	if err != nil {
		return 0, nil
	}
	return binary.LittleEndian.Uint32(b), nil
}

func ReadUInt32BE(r io.Reader) (uint32, error) {
	b := make([]byte, 4)
	_, err := io.ReadFull(r, b)
	if err != nil {
		return 0, nil
	}
	return binary.BigEndian.Uint32(b), nil
}

func WriteByte(data byte, w io.Writer) (int, error) {
	b := make([]byte, 1)
	b[0] = byte(data)
	return w.Write(b)
}

func WriteBytes(data []byte, w io.Writer) (int, error) {
	return w.Write(data)
}

func WriteUInt8(data uint8, w io.Writer) (int, error) {
	b := make([]byte, 1)
	b[0] = byte(data)
	return w.Write(b)
}

func WriteUInt16BE(data uint16, w io.Writer) (int, error) {
	b := make([]byte, 2)
	binary.BigEndian.PutUint16(b, data)
	return w.Write(b)
}

func WriteUInt16LE(data uint16, w io.Writer) (int, error) {
	b := make([]byte, 2)
	binary.LittleEndian.PutUint16(b, data)
	return w.Write(b)
}

func WriteUInt32LE(data uint32, w io.Writer) (int, error) {
	b := make([]byte, 4)
	binary.LittleEndian.PutUint32(b, data)
	return w.Write(b)
}

func WriteUInt32BE(data uint32, w io.Writer) (int, error) {
	b := make([]byte, 4)
	binary.BigEndian.PutUint32(b, data)
	return w.Write(b)
}

func PutUint16BE(data uint16) (uint8, uint8) {
	b := make([]byte, 2)
	binary.BigEndian.PutUint16(b, data)
	return uint8(b[0]), uint8(b[1])
}

func Uint16BE(d0, d1 uint8) uint16 {
	b := make([]byte, 2)
	b[0] = d0
	b[1] = d1

	return binary.BigEndian.Uint16(b)
}

func RGB565ToRGB(data uint16) (r, g, b uint8) {
	r = uint8(data & 0xF800 >> 8)
	g = uint8(data & 0x07E0 >> 3)
	b = uint8(data & 0x001F << 3)

	return
}
func RGB555ToRGB(data uint16) (r, g, b uint8) {
	r = uint8(data & 0x7C00 >> 7)
	g = uint8(data & 0x03E0 >> 2)
	b = uint8(data & 0x001F << 3)

	return
}
func RGBToRGB565(r, g, b uint8) uint16 {
	// 将 8 位的红色值右移 3 位，只保留高 5 位
	r5 := uint16(r>>3) << 11
	// 将 8 位的绿色值右移 2 位，只保留高 6 位
	g6 := uint16(g>>2) << 5
	// 将 8 位的蓝色值右移 3 位，只保留高 5 位
	b5 := uint16(b >> 3)
	// 将处理后的红、绿、蓝值合并成一个 16 位的 RGB565 值
	return r5 | g6 | b5
}

// RGBToRGB555 将 RGB 颜色值转换为 RGB555 格式
func RGBToRGB555(r, g, b uint8) uint16 {
	// 将 8 位的红色值右移 3 位，只保留高 5 位
	r5 := uint16(r>>3) << 10
	// 将 8 位的绿色值右移 3 位，只保留高 5 位
	g5 := uint16(g>>3) << 5
	// 将 8 位的蓝色值右移 3 位，只保留高 5 位
	b5 := uint16(b >> 3)
	// 将处理后的红、绿、蓝值合并成一个 16 位的 RGB555 值
	return r5 | g5 | b5
}
