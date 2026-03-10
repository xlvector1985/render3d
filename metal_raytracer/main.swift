import Metal
import MetalKit
import CoreGraphics
import ImageIO

// 1. Initialize Metal Device
guard let device = MTLCreateSystemDefaultDevice() else {
    fatalError("Metal is not supported on this device")
}

// 2. Load Shader Library
let library: MTLLibrary
do {
    // Read shader source from file
    let shaderPath = "metal_raytracer/raytracer.metal"
    let shaderSource = try String(contentsOfFile: shaderPath, encoding: .utf8)
    library = try device.makeLibrary(source: shaderSource, options: nil)
} catch {
    fatalError("Could not load shader source: \(error)")
}

guard let kernelFunction = library.makeFunction(name: "render") else {
    fatalError("Could not find kernel function 'render'")
}

let pipelineState: MTLComputePipelineState
do {
    pipelineState = try device.makeComputePipelineState(function: kernelFunction)
} catch {
    fatalError("Could not create pipeline state: \(error)")
}

let commandQueue = device.makeCommandQueue()!

// 3. Create Output Texture
let width = 1920
let height = 1080

let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(
    pixelFormat: .rgba32Float,
    width: width,
    height: height,
    mipmapped: false
)
textureDescriptor.usage = [.shaderWrite, .shaderRead]

guard let texture = device.makeTexture(descriptor: textureDescriptor) else {
    fatalError("Could not create texture")
}

// 4. Dispatch Threads
guard let commandBuffer = commandQueue.makeCommandBuffer(),
      let computeEncoder = commandBuffer.makeComputeCommandEncoder() else {
    fatalError("Could not create command buffer/encoder")
}

computeEncoder.setComputePipelineState(pipelineState)
computeEncoder.setTexture(texture, index: 0)

let threadsPerGrid = MTLSize(width: width, height: height, depth: 1)
// Standard threadgroup size
let w = pipelineState.threadExecutionWidth
let h = pipelineState.maxTotalThreadsPerThreadgroup / w
let threadsPerThreadgroup = MTLSize(width: w, height: h, depth: 1)

computeEncoder.dispatchThreads(threadsPerGrid, threadsPerThreadgroup: threadsPerThreadgroup)
computeEncoder.endEncoding()

// 5. Commit and Wait
commandBuffer.commit()
commandBuffer.waitUntilCompleted()

// 6. Read Texture Data and Save Image
let bytesPerPixel = 16 // 4 floats * 4 bytes
let bytesPerRow = width * bytesPerPixel
var data = [Float](repeating: 0, count: width * height * 4)

let region = MTLRegionMake2D(0, 0, width, height)
texture.getBytes(&data, bytesPerRow: bytesPerRow, from: region, mipmapLevel: 0)

// Convert Float buffer to UInt8 buffer for PNG
var uint8Data = [UInt8](repeating: 0, count: width * height * 4)
for i in 0..<(width * height) {
    let r = UInt8(max(0, min(255, data[i*4 + 0] * 255.0)))
    let g = UInt8(max(0, min(255, data[i*4 + 1] * 255.0)))
    let b = UInt8(max(0, min(255, data[i*4 + 2] * 255.0)))
    let a = UInt8(255)
    
    uint8Data[i*4 + 0] = r
    uint8Data[i*4 + 1] = g
    uint8Data[i*4 + 2] = b
    uint8Data[i*4 + 3] = a
}

// Create CGImage
let colorSpace = CGColorSpaceCreateDeviceRGB()
let bitmapInfo = CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue)
guard let provider = CGDataProvider(data: Data(uint8Data) as CFData) else {
    fatalError("Could not create data provider")
}

guard let cgImage = CGImage(
    width: width,
    height: height,
    bitsPerComponent: 8,
    bitsPerPixel: 32,
    bytesPerRow: width * 4,
    space: colorSpace,
    bitmapInfo: bitmapInfo,
    provider: provider,
    decode: nil,
    shouldInterpolate: false,
    intent: .defaultIntent
) else {
    fatalError("Could not create CGImage")
}

// Save to Disk
let url = URL(fileURLWithPath: "metal_render.png")
guard let destination = CGImageDestinationCreateWithURL(url as CFURL, kUTTypePNG, 1, nil) else {
    fatalError("Could not create image destination")
}

CGImageDestinationAddImage(destination, cgImage, nil)
if CGImageDestinationFinalize(destination) {
    print("Successfully saved metal_render.png")
} else {
    print("Failed to save image")
}
