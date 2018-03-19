module FFImage
  class LibGIF
    def initialize(buf)
      @buf = buf
    end

    def dimensions
      header = @buf[0..10].unpack("lsss")
      header[0] == 0x38464947 or raise "Not a GIF"
      header[1] == 0x6137 || header[1] == 0x6139 or raise "Not a GIF"
      [header[2], header[3]]
    end
  end
end
