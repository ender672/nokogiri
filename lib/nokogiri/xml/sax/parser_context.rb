module Nokogiri
  module XML
    module SAX
      ###
      # Context for XML SAX parsers.  This class is usually not instantiated
      # by the user.  Instead, you should be looking at
      # Nokogiri::XML::SAX::Parser
      class ParserContext
        attr_accessor :replace_entities

        def initialize(thing, encoding = 'UTF-8')
          @thing = thing
          @encoding = encoding
          @parser = nil
          @replace_entities = nil
        end

        def self.memory(data, *args)
          raise ArgumentError if data.nil?
          raise "data cannot be empty" if data.empty?

          new(data, *args)
        end

        def self.file(path, encoding=nil)
          new(File.open(path, 'r'), encoding)
        end

        def self.io(new_io, encoding)
          new(new_io, encoding)
        end

        def parse_with(parser)
          raise ArgumentError if parser.nil?
          @parser = parser
          @parser.replace_entities = @replace_entities if @replace_entities
          @parser.parse(@thing)
          nil
        end

        def line(*args)
          @parser ? @parser.line(*args) : 0
        end

        def column(*args)
          @parser ? @parser.column(*args) : 0
        end
      end
    end
  end
end
