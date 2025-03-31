package common

import (
	"FuckRDP/grdp/core"
	"encoding/json"
	"fmt"
	"image"
	"image/color"
	"image/png"
	"log"
	"math/rand"
	"os"
	"testing"
	"time"
)

type bitmapStruct struct {
	// 位图坐标信息
	DestLeft   int
	DestTop    int
	DestRight  int
	DestBottom int

	// 位图尺寸信息
	Width  int
	Height int

	Data []byte

	BitsPerPixel int
}

// 更新像素值
func updatePixel(pixel *uint8, diff int) {
	newValue := int(*pixel) + diff
	if newValue < 0 {
		newValue = 0
	} else if newValue > 255 {
		newValue = 255
	}
	*pixel = uint8(newValue)
}

// EmbedData 函数使用 EMD 算法将 data 隐写到 originImg 的 R 和 G 通道的高 5 位中
func EmbedData(originImg []color.RGBA, data []byte) ([]color.RGBA, bool) {
	//if len(data)*8 > len(originImg) {
	//	return nil, false
	//}

	bitIndex := 0
	byteIndex := 0

	for i := 0; i < len(originImg); i++ {
		if byteIndex >= len(data) {
			break
		}

		currentByte := data[byteIndex]
		bitB := (currentByte >> (7 - bitIndex)) & 1
		bitR := (currentByte >> (7 - (bitIndex + 1))) & 1

		b := originImg[i].B
		r := originImg[i].R

		bHigh5 := (b >> 3) & 0x1F
		rHigh5 := (r >> 3) & 0x1F

		bDiff := int(bHigh5) % 2
		rDiff := int(rHigh5) % 2

		if (bitB == 0 && bDiff != 0) || (bitB == 1 && bDiff == 0) {
			//fmt.Println("B 1:", b)
			if bHigh5 > 0 {
				updatePixel(&b, -1<<3)
			} else {
				updatePixel(&b, 1<<3)
			}
			//fmt.Println("B 2:", b)
		}

		if (bitR == 0 && rDiff != 0) || (bitR == 1 && rDiff == 0) {
			if rHigh5 > 0 {
				updatePixel(&r, -1<<3)
			} else {
				updatePixel(&r, 1<<3)
			}
		}

		originImg[i].B = b
		originImg[i].R = r

		bitIndex += 2
		if bitIndex >= 8 {
			bitIndex = 0
			byteIndex++
		}
	}

	return originImg, true
}

func TryExtractData(originImg []byte, left, top, width, height, bitsPerPixel int) []byte {
	var (
		i = 0
		r uint8
		g uint8
		b uint8
		a uint8
	)
	var (
		bitIndex          = 0
		byteIndex         = 0
		magicCode1 uint8  = 0
		magicCode2 uint8  = 0
		dataSize1  uint8  = 0
		dataSize2  uint8  = 0
		hasData           = true
		data       []byte = nil
	)

	pixels := image.NewRGBA(image.Rect(0, 0, width, height))

	extractFunc := func(b, r uint8) {
		bHigh5 := (b >> 3) & 0x1F
		rHigh5 := (r >> 3) & 0x1F

		bitB := int(bHigh5) % 2
		bitR := int(rHigh5) % 2

		switch byteIndex {
		case 0:
			magicCode1 |= byte(bitB) << (7 - bitIndex)
			magicCode1 |= byte(bitR) << (7 - (bitIndex + 1))
		case 1:
			magicCode2 |= byte(bitB) << (7 - bitIndex)
			magicCode2 |= byte(bitR) << (7 - (bitIndex + 1))
		case 2:
			fmt.Println(magicCode1, magicCode2)
			if magicCode1 != 0xde || magicCode2 != 0xad {
				hasData = false
				return
			}
			dataSize1 |= byte(bitB) << (7 - bitIndex)
			dataSize1 |= byte(bitR) << (7 - (bitIndex + 1))
		case 3:
			dataSize2 |= byte(bitB) << (7 - bitIndex)
			dataSize2 |= byte(bitR) << (7 - (bitIndex + 1))
		case 4:
			if data == nil {
				fmt.Println(dataSize1, dataSize2)
				data = make([]byte, int(dataSize1)<<8+int(dataSize2)+4) // 4 for CRC32
			}
			fallthrough
		default:
			data[byteIndex-4] |= byte(bitB) << (7 - bitIndex)
			data[byteIndex-4] |= byte(bitR) << (7 - (bitIndex + 1))
		}
		bitIndex += 2
		if bitIndex >= 8 {
			bitIndex = 0
			byteIndex++
		}
		if hasData && data != nil && byteIndex-4 >= cap(data) {
			hasData = false
		}
	}
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			r, g, b, a = ToRGBA(bitsPerPixel, i, originImg)
			pixels.SetRGBA(x, y, color.RGBA{R: r, G: g, B: b, A: a})
			i += bitsPerPixel
			if hasData {
				extractFunc(b, r)
			}
		}
	}
	return data
}

// ExtractData 函数从 originImg 的 R 和 G 通道的高 5 位中读取使用 EMD 算法嵌入的数据
func ExtractData(originImg []color.RGBA, dataLen int) ([]byte, bool) {
	result := make([]byte, dataLen)
	bitIndex := 0
	byteIndex := 0

	for i := 0; i < len(originImg); i++ {
		if byteIndex >= dataLen {
			break
		}

		b := originImg[i].B
		r := originImg[i].R

		bHigh5 := (b >> 3) & 0x1F
		rHigh5 := (r >> 3) & 0x1F

		bitB := int(bHigh5) % 2
		bitR := int(rHigh5) % 2

		result[byteIndex] |= byte(bitB) << (7 - bitIndex)
		result[byteIndex] |= byte(bitR) << (7 - (bitIndex + 1))

		bitIndex += 2
		if bitIndex >= 8 {
			bitIndex = 0
			byteIndex++
		}
	}

	return result, true
}

// 将结构体序列化到文件
func saveToFile(bitmap *bitmapStruct, filename string) error {
	file, err := os.Create(filename)
	if err != nil {
		return err
	}
	defer file.Close()

	encoder := json.NewEncoder(file)
	return encoder.Encode(bitmap)
}

// 从文件中反序列化结构体
func loadFromFile(filename string) (bitmapStruct, error) {
	var bitmap bitmapStruct
	file, err := os.Open(filename)
	if err != nil {
		return bitmap, err
	}
	defer file.Close()

	decoder := json.NewDecoder(file)
	err = decoder.Decode(&bitmap)
	return bitmap, err
}

// readPNGToRGBA 打开指定路径的 PNG 文件，并返回图像数据的 []color.RGBA 切片
func readPNGToRGBA(path string) ([]color.RGBA, int, int, error) {
	// 打开文件
	file, err := os.Open(path)
	if err != nil {
		return nil, 0, 0, err
	}
	defer file.Close()

	// 解码 PNG 图像
	img, err := png.Decode(file)
	if err != nil {
		return nil, 0, 0, err
	}

	// 获取图像的边界
	bounds := img.Bounds()
	width := bounds.Dx()
	height := bounds.Dy()

	// 创建一个 []color.RGBA 切片来存储图像数据
	rgbaData := make([]color.RGBA, width*height)

	// 遍历图像的每个像素
	index := 0
	for y := bounds.Min.Y; y < bounds.Max.Y; y++ {
		for x := bounds.Min.X; x < bounds.Max.X; x++ {
			// 获取当前像素的颜色
			rgba := color.RGBAModel.Convert(img.At(x, y)).(color.RGBA)
			// 将颜色添加到切片中
			rgbaData[index] = rgba
			index++
		}
	}

	return rgbaData, width, height, nil
}

func bitsToRGBA(originImg []byte, width, height, bitsPerPixel int, ch chan color.RGBA) {
	var (
		i = 0
		r uint8
		g uint8
		b uint8
		a uint8
	)
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			r, g, b, a = ToRGBA(bitsPerPixel, i, originImg)
			ch <- color.RGBA{R: r, G: g, B: b, A: a}

			i += bitsPerPixel
		}
	}

	close(ch)
}

func bitmap2Colors(bitmap bitmapStruct) []color.RGBA {
	colorChan := make(chan color.RGBA)
	go bitsToRGBA(bitmap.Data, bitmap.Width, bitmap.Height, bitmap.BitsPerPixel, colorChan)
	var cc []color.RGBA
	for rgba := range colorChan {
		cc = append(cc, rgba)
	}
	return cc
}
func copyRGBs(cc []color.RGBA) []color.RGBA {
	ccCopy := make([]color.RGBA, len(cc))
	// 使用 copy 函数复制切片
	copy(ccCopy, cc)
	return ccCopy
}
func rgb2Byte(cc []color.RGBA) []byte {
	var wtf []byte
	for _, rgb := range cc {
		rgb565 := core.RGBToRGB565(rgb.R, rgb.G, rgb.B)
		a, b := core.PutUint16BE(rgb565)
		wtf = append(wtf, a, b)
	}
	return wtf
}
func byte2Rgb(cc []byte, width, height int) []color.RGBA {
	colorChan := make(chan color.RGBA)
	go bitsToRGBA(cc, width, height, 2, colorChan)
	var rgbs []color.RGBA
	for rgba := range colorChan {
		rgbs = append(rgbs, rgba)
	}
	return rgbs
}
func TestExtract(t *testing.T) {
	bitmap, err := loadFromFile("F:\\GolandProjects\\FuckRDP\\wtf-embed.txt")
	if err != nil {
		t.Error(err)
	}
	embedRGBs := bitmap2Colors(bitmap)
	embedData, _ := ExtractData(embedRGBs, 2)
	log.Println(embedData)
}
func TestTryExtract(t *testing.T) {
	bitmap, err := loadFromFile("F:\\GolandProjects\\FuckRDP\\wtf-embed.txt")
	if err != nil {
		t.Error(err)
	}
	embedData := TryExtractData(bitmap.Data, 0, 0, bitmap.Width, bitmap.Height, bitmap.BitsPerPixel)
	log.Println("TryExtractData", embedData)
}
func TestEmbed(t *testing.T) {
	bitmap, err := loadFromFile("F:\\GolandProjects\\FuckRDP\\wtf.txt")
	if err != nil {
		t.Error(err)
	}
	originRGBs := bitmap2Colors(bitmap)
	originRGBs, bitmap.Width, bitmap.Height, _ = readPNGToRGBA("C:\\Users\\simon-desktop\\Desktop\\218d3afb146cd9e6cf66967b7c0fd1e4.png")

	embedRGBs, _ := EmbedData(originRGBs, []byte{0xde, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0x03, 0xde, 0xad, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03, 0xde, 0xad, 0x00, 0x01, 0x00, 0x01, 0x03, 0xad, 0x00, 0x01, 0x03})

	bitmap.Data = rgb2Byte(embedRGBs)
	_ = saveToFile(&bitmap, "F:\\GolandProjects\\FuckRDP\\wtf-embed.txt")
}
func TestDraw(t *testing.T) {
	bitmap, err := loadFromFile("F:\\GolandProjects\\FuckRDP\\wtf-embed.txt")
	if err != nil {
		t.Error(err)
	}
	pixels := image.NewRGBA(image.Rect(0, 0, bitmap.Width, bitmap.Height))
	originRGBs := bitmap2Colors(bitmap)
	i := 0
	for y := 0; y < bitmap.Height; y++ {
		for x := 0; x < bitmap.Width; x++ {
			pixels.Set(x, y, originRGBs[i])
			i++
		}
	}

	outFile, err := os.Create("F:\\GolandProjects\\FuckRDP\\wtf-draw.png")
	if err != nil {
		log.Printf("创建文件失败: %v", err)
	}
	defer outFile.Close()
	err = png.Encode(outFile, pixels)
	if err != nil {
		log.Printf("保存文件失败: %v", err)
	}
}
func TestEmbedExtractDraw(t *testing.T) {
	TestEmbed(t)
	TestTryExtract(t)
	TestDraw(t)
}

func TestDumpBitmap(t *testing.T) {
	bitmap, err := loadFromFile("F:\\GolandProjects\\FuckRDP\\www.txt")
	if err != nil {
		t.Error(err)
	}
	fmt.Println("bpp", bitmap.BitsPerPixel)
	originRGBs := bitmap2Colors(bitmap)
	pixelSize := 4 * 4
	bitmap.Data = rgb2Byte(originRGBs[:pixelSize])
	PrintBytesLikeC(bitmap.Data)
	fmt.Println(len(bitmap.Data))

	pixels := image.NewRGBA(image.Rect(0, 0, 8, 8))
	i := 0
	for y := 0; y < 4; y++ {
		for x := 0; x < 4; x++ {
			pixels.Set(x, y, originRGBs[i])
			i++
		}
	}
	outFile, _ := os.Create("F:\\GolandProjects\\FuckRDP\\dump-draw.png")
	_ = png.Encode(outFile, pixels)
	for i2, rgb := range originRGBs {
		if i2 > 15 {
			continue
		}
		fmt.Println("rgb ", i2, rgb.R, rgb.G, rgb.B)
	}
}
func TestEmbedDumpBitmap(t *testing.T) {
	bitmap, err := loadFromFile("F:\\GolandProjects\\FuckRDP\\wtf.txt")
	if err != nil {
		t.Error(err)
	}
	originRGBs := bitmap2Colors(bitmap)
	data, _ := EmbedData(originRGBs[:16], []byte{0xde, 0xad})
	embed := rgb2Byte(data[:16])
	PrintBytesLikeC(embed)
	extractData, _ := ExtractData(data[:16], 2)
	PrintBytesLikeC(extractData)
}

// PrintBytesLikeC 函数将 []byte 切片以 C 语言代码样式打印
func PrintBytesLikeC(bytes []byte) {
	fmt.Print("{ ")
	for i, b := range bytes {
		if i > 0 {
			fmt.Print(", ")
		}
		fmt.Printf("0x%02X", b)
	}
	fmt.Println(" }")
}
func TestTryDumpExtract(t *testing.T) {
	bitmap := bitmapStruct{
		DestLeft:     0,
		DestTop:      0,
		DestRight:    0,
		DestBottom:   0,
		Width:        4,
		Height:       4,
		Data:         []byte{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
		BitsPerPixel: 2,
	}
	data := TryExtractData(bitmap.Data, 0, 0, 4, 4, 2)
	fmt.Println(data)
}
func TestRGB(t *testing.T) {
	rgb565 := core.RGBToRGB565(255, 255, 255)
	a, b := core.PutUint16BE(rgb565)
	log.Println(a, b)
	r, g, u := core.RGB565ToRGB(rgb565)
	log.Println(r, g, u)
}

// 对于64*64的图片，可以嵌入64*64/4 = 1024个byte
func TestEmbedAt25_03_30(t *testing.T) {
	var plainText = make([]byte, 1024)
	for i := range plainText {
		plainText[i] = byte(i)
	}
	var plainImg = make([]color.RGBA, 64*64)
	for i := range plainImg {
		plainImg[i] = color.RGBA{
			R: 0,
			G: 255,
			B: 0,
			A: 255,
		}
	}
	embedImg, _ := EmbedData(plainImg, plainText)
	embedBmp := drawColorToBmp(64, 64, embedImg)
	outFile, _ := os.Create("F:\\GolandProjects\\FuckRDP\\TestEmbedAt25_03_30-embed.png")
	_ = png.Encode(outFile, embedBmp)
}
func TestExtractAt25_03_30(t *testing.T) {
	var plainText = make([]byte, 1024)
	for i := range plainText {
		plainText[i] = byte(i)
	}
	var plainImg = make([]color.RGBA, 64*64)
	for i := range plainImg {
		plainImg[i] = color.RGBA{
			R: 255,
			G: 255,
			B: 255,
			A: 255,
		}
	}
	embedImg, _ := EmbedData(plainImg, plainText)

	extractData, _ := ExtractData(embedImg, 1024)

	for i, d := range extractData {
		if byte(i) != d {
			log.Printf("wtf in %d is %d.\n,", i, d)
		}
	}
}
func TestExtractFromStreamAt25_03_30(t *testing.T) {
	var plainText = make([]byte, 1024)
	for i := range plainText {
		plainText[i] = byte(i)
	}
	for i := 0; i < 1024; i += 3 {
		plainText[i] = 222
		plainText[i+1] = 252
		plainText[i+2] = 248 // success
	}
	var plainImg = make([]color.RGBA, 64*64)
	for i := range plainImg {
		plainImg[i] = color.RGBA{
			R: 255,
			G: 255,
			B: 255,
			A: 255,
		}
	}
	embedImg, _ := EmbedData(plainImg, plainText)

	embedImgStream := rgb2Byte(embedImg)
	readFromStream := byte2Rgb(embedImgStream, 64, 64)

	extractData, _ := ExtractData(readFromStream, 1024)
	log.Println(extractData)
}
func TestExtractRandomFromStreamAt25_03_30(t *testing.T) {
	var plainText = make([]byte, 1024)
	for i := range plainText {
		plainText[i] = byte(i)
	}
	var plainImg = GenerateRandomRGBAArray(64 * 64)
	embedImg, _ := EmbedData(plainImg, plainText)

	embedImgStream := rgb2Byte(embedImg)
	readFromStream := byte2Rgb(embedImgStream, 64, 64)

	extractData, _ := ExtractData(readFromStream, 1024)
	for i, d := range extractData {
		if byte(i) != d {
			log.Printf("wtf in %d is %d.\n,", i, d)
		}
	}
}
func drawColorToBmp(width, height int, rgb []color.RGBA) *image.RGBA {
	bmp := image.NewRGBA(image.Rect(0, 0, width, height))
	i := 0
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			bmp.Set(x, y, rgb[i])
			i++
		}
	}
	return bmp
}
func GenerateRandomRGBAArray(length int) []color.RGBA {
	// 初始化随机数种子
	rand.Seed(time.Now().UnixNano())

	// 创建一个长度为 length 的 color.RGBA 数组
	colors := make([]color.RGBA, length)

	// 遍历数组，为每个元素生成随机的 RGBA 值
	for i := 0; i < length; i++ {
		r := uint8(rand.Intn(256))
		g := uint8(rand.Intn(256))
		b := uint8(rand.Intn(256))
		a := uint8(rand.Intn(256))

		colors[i] = color.RGBA{R: r, G: g, B: b, A: a}
	}

	return colors
}
