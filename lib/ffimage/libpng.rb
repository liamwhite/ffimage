module FFImage
  class LibPNG
    def initialize(buf)
      @buf = buf
    end

    def dimensions
      header = @buf[0..24].unpack("NNNNNN")
      header[0] == 0x89504e47 or raise "Not a PNG"
      header[1] == 0x0d0a1a0a or raise "Not a PNG"

      ihdr_length = header[2]
      header[3] = 0x49484452 or raise "Missing or invalid IHDR" # "IHDR"

      [header[4], header[5]]
    end
  end
end
