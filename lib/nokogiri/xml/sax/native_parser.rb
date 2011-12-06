module Nokogiri
  module XML
    module SAX
      class NativeParser
        class Attribute < Struct.new(:localname, :prefix, :uri, :value)
        end
      end
    end
  end
end
