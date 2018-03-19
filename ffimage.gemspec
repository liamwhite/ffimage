require_relative 'lib/ffimage/version'

Gem::Specification.new do |s|
  s.name                  = "ffimage"
  s.version               = FFImage::VERSION
  s.summary               = 'C bindings for image attribute detection and manipulation'
  s.author                = 'Liam P. White'
  s.files                 += Dir.glob("lib/**/*.rb")
  s.files                 += Dir.glob("ext/**/*.[ch]")
  s.extensions            = ['ext/ffimage/extconf.rb']
  s.required_ruby_version = '>= 2.3.6'
end
