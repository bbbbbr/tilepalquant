// == Uncompressed Indexed PNG Export ==


// The PNG uncompressed indexed color file structure in this implementation:
//
// Only 1x of each PNG chunk type is used
//
//  - Signature (8 bytes)
//  - IHDR chunk
//  - PLTE chunk [Palette data]
//  - IDAT chunk [Indexed Pixel dat]
//    - IDAT payload length (4 bytes)
//    - IDAT chunk type     (4 bytes)
//    - IDAT Payload
//      - Zlib header (2 bytes)
//      - Deflate chunks [One per pixel row]
//        - Final/Non-final indicator (1 byte)
//        - Deflate Length            (2 bytes)
//        - Deflate Length xor FF     (2 bytes)
//          - PNG single Scanline of Row data
//            - Row start filter[0 for none] (1 bytes)
//            - Indexed Pixel Row Data       (width's worth of bytes)
//      - Zlib Adler checksum (4 bytes)
//    - IDAT CRC-32 of chunk type and payload (4 bytes)
//  - IEND chunk


const PNG_PAL_RGB888_SZ      = 3;
const PNG_PAL_RGBA8888_SZ    = 4;

const PNG_BIT_DEPTH_8        = 8;
const PNG_COLOR_TYPE_INDEXED = 3;
const PNG_COMPRESSION_METHOD_DEFLATE_NONE = 0;
const PNG_FILTER_METHOD_NONE = 0;
const PNG_INTERLACING_NONE   = 0;

const PNG_CHUNK_LENGTH_SZ   = 4;
const PNG_CHUNK_TYPE_SZ     = 4;
const PNG_CHUNK_CHECKSUM_SZ = 4;
const PNG_CHUNK_OVERHEAD    = PNG_CHUNK_LENGTH_SZ + PNG_CHUNK_TYPE_SZ + PNG_CHUNK_CHECKSUM_SZ;
const PNG_SIGNATURE_SZ      = 8;
const PNG_IHDR_SZ           = 13;
const PNG_IEND_SZ           = 0;

const PNG_IDAT_BYTE_SZ      = 8192;

const PNG_ROW_FILTER_TYPE_SZ   = 1;
const PNG_ROW_FILTER_TYPE_NONE = 0;

const PNG_EXPORT_SUPPORTED_MAX_WIDTH = 65535;

const ZLIB_HEADER_SZ           = 2;    // CMF, FLG
const ZLIB_FOOTER_SZ           = 4;    // 4 byte Adler checksum
const ZLIB_HEADER_CMF          = 0x78; // CMF: Compression Method: No Compression
const ZLIB_HEADER_FLG          = 0x01; // FLG: Flags: FCHECK, No DICT, FLEVEL = 0 (fastest)

const DEFLATE_HEADER_FINAL_NO  = 0;    // BTYPE Uncompressed, not final
const DEFLATE_HEADER_FINAL_YES = 1;    // BTYPE Uncompressed, final
const DEFLATE_HEADER_SZ        = 5;    // 1 byte Is Final block, 2 bytes Length, 2 bytes 


var HiAttribEnabled = false;
var HiAttribNuMColorsPerPalette;

export function pngExportSetHiAttribMode(modEnabled, palSize) {
    HiAttribEnabled = modEnabled;
    HiAttribNuMColorsPerPalette = palSize;
}


function uint8ToBase64(arr) {
    return btoa(Array(arr.length)
        .fill("")
        .map((_, i) => String.fromCharCode(arr[i]))
        .join(""));
}


function write16Le(outBuffer, index, value) {
    outBuffer[index] = value % 256;
    value = Math.floor(value / 256);
    outBuffer[index + 1] = value % 256;
}
function write32Be(outBuffer, index, value) {
    outBuffer[index + 3] = value % 256;
    value = Math.floor(value / 256);
    outBuffer[index + 2] = value % 256;
    value = Math.floor(value / 256);
    outBuffer[index + 1] = value % 256;
    value = Math.floor(value / 256);
    outBuffer[index + 0] = value % 256;
}
function write16Be(outBuffer, index, value) {
    outBuffer[index + 1] = value % 256;
    value = Math.floor(value / 256);
    outBuffer[index + 0] = value % 256;
}


var zlib_adler_a;
var zlib_adler_b;
function adler_reset() {
    zlib_adler_a = 1, zlib_adler_b = 0;
}
function adler_crc_update(buffer) {

    for (var i = 0; i < buffer.length; i++ ) {
         zlib_adler_a = (zlib_adler_a + buffer[i]) % 65521;
         zlib_adler_b = (zlib_adler_b + zlib_adler_a) % 65521;
    }
}


var crcTable;
function makeCRCTable(){

    var c;
    var crcTable = [];
    for(var n =0; n < 256; n++){
        c = n;
        for(var k =0; k < 8; k++){
            c = ((c&1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1));
        }
        crcTable[n] = c;
    }
    return crcTable;
}
function crc32(buffer) {

    // Make CRC table if not already populated
    if (!crcTable) crcTable = makeCRCTable();
    var crc = 0 ^ (-1);

    for (var i = 0; i < buffer.length; i++ ) {
        crc = (crc >>> 8) ^ crcTable[(crc ^ buffer[i]) & 0xFF];
    }

    return (crc ^ (-1)) >>> 0;
}


function pngWriteChunk(output, index, type, payload) {  

    // Write the Length of Payload (does not include length, type, checksum)
    write32Be(output, index, payload.length);
    index += PNG_CHUNK_LENGTH_SZ;
    // Data covered by CRC starts immediately after length field
    let crc_index_start = index;

    // Write the type
    const typeData = new TextEncoder().encode(type);
    output.set(typeData, index);
    index += PNG_CHUNK_TYPE_SZ;

    // Write the payload
    output.set(payload, index);
    index += payload.length;

    // Write the checksum of type + payload
    const checksum = crc32(output.slice(crc_index_start, index));
    write32Be(output, index, checksum);
    index += PNG_CHUNK_CHECKSUM_SZ;

    return index;
}
function pngPreparePaletteData(paletteData, totalPaletteColors) {

    // Reformat palette to RGB888 from BGRA8888
    const paletteDataRGB888 = new Uint8Array(totalPaletteColors * PNG_PAL_RGB888_SZ);
    let srcIdx = 0, dstIdx = 0;
    while (srcIdx < (totalPaletteColors * PNG_PAL_RGB888_SZ)) {
        paletteDataRGB888[srcIdx    ] = paletteData[dstIdx + 2];
        paletteDataRGB888[srcIdx + 1] = paletteData[dstIdx + 1];
        paletteDataRGB888[srcIdx + 2] = paletteData[dstIdx];
        srcIdx += 3;
        dstIdx += 4; // Discard every 4th source byte
    }

    return paletteDataRGB888;
}
function pngPrepareIndexedPixelData(width, height, colorIndexes) {

    // For Indexed Pixel data, encode each scanline row as a separate
    // zlib chunk which makes it easier to pack it all together.
    // Tuck the PNG row data into that
    const deflate_chunk_sz  = PNG_ROW_FILTER_TYPE_SZ + width;
    const zlib_row_chunk_sz = DEFLATE_HEADER_SZ + deflate_chunk_sz;
    const zlib_total_size   = ZLIB_HEADER_SZ + (zlib_row_chunk_sz * height) + ZLIB_FOOTER_SZ;

    // zlib/Deflate Adler checksum is only on the 
    // Size of block in little endian and its 1's complement (4 bytes) 
    adler_reset();
    const zlibPixelRows = new Uint8Array(zlib_total_size);
    let zlibIdx = 0;
    let pixelSourceIdx = 0;

    // Write zlib header bytes
    zlibPixelRows[zlibIdx++] = ZLIB_HEADER_CMF;
    zlibPixelRows[zlibIdx++] = ZLIB_HEADER_FLG;
    // Write out the scanline pixel index rows
    for (let y = 0; y < height; y++) {
        // Deflate Header
        zlibPixelRows[zlibIdx++] = (y == height -1) ? DEFLATE_HEADER_FINAL_YES :  DEFLATE_HEADER_FINAL_NO;
        write16Le(zlibPixelRows, zlibIdx, deflate_chunk_sz);          zlibIdx += 2;
        write16Le(zlibPixelRows, zlibIdx, deflate_chunk_sz ^ 0xFFFF); zlibIdx += 2;

        // PNG Row filter header + row data
        let adler_start = zlibIdx;
        zlibPixelRows[zlibIdx++] = PNG_ROW_FILTER_TYPE_NONE;
        for (let x = 0; x < width; x++) {

            if (HiAttribEnabled)
                zlibPixelRows[zlibIdx++] = colorIndexes[pixelSourceIdx++] % HiAttribNuMColorsPerPalette;
            else
                zlibPixelRows[zlibIdx++] = colorIndexes[pixelSourceIdx++];
        }
        adler_crc_update(zlibPixelRows.slice(adler_start, zlibIdx));
    }
    // Write zlib Adler crc
    write16Be(zlibPixelRows, zlibIdx, zlib_adler_b); zlibIdx += 2;
    write16Be(zlibPixelRows, zlibIdx, zlib_adler_a); zlibIdx += 2;

    return zlibPixelRows;
}


// Expects:
// - paletteData:  BGRA8888 (4 bytes, alpha gets discarded), 256 colors max
//                 (See function addExportIndexedColors() for indexed export palette packing)
// - colorIndexes: 1 byte per pixel
export function encodeIndexedPngToBase64(width, height, paletteData, totalPaletteColors, colorIndexes) {

    if (!paletteData || !ArrayBuffer.isView(paletteData) || (paletteData.length % PNG_PAL_RGBA8888_SZ) !== 0)
        throw new Error("Palette entries must RGBA8888 (4 bytes per color) byte view");

    if (!colorIndexes || !ArrayBuffer.isView(colorIndexes))
        throw new Error("Color Indexes must be a byte view");

    if ((totalPaletteColors > 256) || ((totalPaletteColors * PNG_PAL_RGBA8888_SZ) > paletteData.length))
        throw new Error("PNG palettes are limited to 256 entries, and number of colors must not exceed incoming palette array size");

    if (width > PNG_EXPORT_SUPPORTED_MAX_WIDTH)
        throw new Error("Image is wider than maximum supported width of " + PNG_EXPORT_SUPPORTED_MAX_WIDTH);


    // == Prepare Palettes and Indexed Pixel Data into suitable PNG format ==

    // Reformat palette to RGB888 from BGRA8888
    const paletteDataRGB888 = pngPreparePaletteData(paletteData, totalPaletteColors);
    const zlibPixelRows     = pngPrepareIndexedPixelData(width, height, colorIndexes);


    // == Now build the PNG output ==

    // PNG buffer size
    let pngBufLen =  PNG_SIGNATURE_SZ;
        pngBufLen += PNG_IHDR_SZ              + PNG_CHUNK_OVERHEAD;
        pngBufLen += paletteDataRGB888.length + PNG_CHUNK_OVERHEAD; // PLTE
        pngBufLen += zlibPixelRows.length     + PNG_CHUNK_OVERHEAD; // IDAT
        pngBufLen += PNG_IEND_SZ              + PNG_CHUNK_OVERHEAD;

    const pngBuf = new Uint8Array(pngBufLen);
    let pngBufIndex = 0;

    // PNG Signature
    const signature = new Uint8Array([0X89, 0X50, 0X4E, 0X47, 0X0D, 0X0A, 0X1A, 0X0A]);
    pngBuf.set(signature, pngBufIndex);
    pngBufIndex += signature.length;

    // PNG IHDR
    const ihdr = new Uint8Array(13);
    write32Be(ihdr, 0, width);
    write32Be(ihdr, 4, height);  
    ihdr[8]  = PNG_BIT_DEPTH_8;
    ihdr[9]  = PNG_COLOR_TYPE_INDEXED;
    ihdr[10] = PNG_COMPRESSION_METHOD_DEFLATE_NONE;
    ihdr[11] = PNG_FILTER_METHOD_NONE;
    ihdr[12] = PNG_INTERLACING_NONE;
    pngBufIndex = pngWriteChunk(pngBuf, pngBufIndex, "IHDR", ihdr);

    // PNG Indexed Color Palette
    pngBufIndex = pngWriteChunk(pngBuf, pngBufIndex, "PLTE", paletteDataRGB888);

    // PNG Indexed Color Pixel Data (in 8192 byte IDAT Chunks)
    pngBufIndex = pngWriteChunk(pngBuf, pngBufIndex, "IDAT", zlibPixelRows);

    // PNG End of data
    pngBufIndex = pngWriteChunk(pngBuf, pngBufIndex, "IEND", new Uint8Array());

    return "data:image/png;base64," + uint8ToBase64(pngBuf);
};



// Maybe a companion source header file? 
//  - mainly just need "tile height" to infer everything else...
//    - maybe just support 8x1 mode? -> so no header is needed?

// TODO: Move into hi_attrib.js
//
// Expects:
// - colorIndexes: 1 byte per pixel
export function encodeAttributeMapToBase64(width, height, tileWidth, tileHeight, colorIndexes, colorsPerPal) {

    // Attribute buffer sized in tiles
    const attribMap = new Uint8Array((width / tileWidth) * (height / tileHeight));

    let tileY = 0;
    for (let pixelY = 0; pixelY < height; pixelY += tileHeight) {
        let tileX = 0;
        for (let pixelX = 0; pixelX < width; pixelX += tileWidth) {
            const pixelIndex = pixelX + (pixelY * width);
            const mapIndex = tileX + (tileY * (width / tileWidth));
            attribMap[mapIndex] = colorIndexes[pixelIndex] / colorsPerPal;
            // console.log("Tile X, Y: " + tileX + "," + tileY + " | Pixel X, Y: " + pixelX + "," + pixelY + " | Color Index -> Pal: " + colorIndexes[pixelIndex] + "," + colorIndexes[pixelIndex] / colorsPerPal + " | pixelIndex, mapIndex: " + pixelIndex + "," +  mapIndex + " | Result ---> " + attribMap[mapIndex]);
            tileX++;
        }
        tileY++;
    }

    return "data:application/octet-stream;base64," + uint8ToBase64(attribMap);
};
