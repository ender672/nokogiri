module Nokogiri
  module XML
    module SAX
      ###
      # Context for XML SAX parsers.  This class is usually not instantiated
      # by the user.  Instead, you should be looking at
      # Nokogiri::XML::SAX::Parser
      class ParserContext
        def initialize(thing, encoding = 'UTF-8')
          @thing = thing
          @encoding = encoding
          @push_parser = nil
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

          @push_parser = PushParser.new(parser.document)
          parse_with2
        end

        def parse_with2
          @push_parser.replace_entities = @replace_entities if @replace_entities
          yield @push_parser if block_given?

          if @thing.respond_to?(:read)
            while((data = @thing.read) && !data.empty?)
              @push_parser << data rescue nil
            end
            @push_parser.finish rescue nil
          else
            @push_parser.write(@thing, true) rescue nil
          end

          nil
        end

        def line(*args)
          @push_parser ? @push_parser.line(*args) : 0
        end

        def column(*args)
          @push_parser ? @push_parser.column(*args) : 0
        end

        def replace_entities=(arg)
          if @push_parser
            @push_parser.replace_entities = arg
          else
            @replace_entities = arg
          end
        end

        def replace_entities(*args)
          @push_parser ? @push_parser.replace_entities(*args) : @replace_entities
        end
      end
    end
  end
end
