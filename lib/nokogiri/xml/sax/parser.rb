module Nokogiri
  module XML
    module SAX
      ###
      # This parser is a SAX style parser that reads it's input as it
      # deems necessary.  The parser takes a Nokogiri::XML::SAX::Document,
      # an optional encoding, then given an XML input, sends messages to
      # the Nokogiri::XML::SAX::Document.
      #
      # Here is an example of using this parser:
      #
      #   # Create a subclass of Nokogiri::XML::SAX::Document and implement
      #   # the events we care about:
      #   class MyDoc < Nokogiri::XML::SAX::Document
      #     def start_element name, attrs = []
      #       puts "starting: #{name}"
      #     end
      #
      #     def end_element name
      #       puts "ending: #{name}"
      #     end
      #   end
      #
      #   # Create our parser
      #   parser = Nokogiri::XML::SAX::Parser.new(MyDoc.new)
      #
      #   # Send some XML to the parser
      #   parser.parse(File.read(ARGV[0]))
      #
      # For more information about SAX parsers, see Nokogiri::XML::SAX.  Also
      # see Nokogiri::XML::SAX::Document for the available events.
      class Parser
        # The Nokogiri::XML::SAX::Document where events will be sent.
        attr_accessor :document

        # The encoding beings used for this document.
        attr_accessor :encoding

        # Create a new Parser with +doc+ and +encoding+
        def initialize doc = Nokogiri::XML::SAX::Document.new, encoding = 'UTF-8'
          @encoding = encoding
          @document = doc
          @filename = nil
        end

        ###
        # Parse given +thing+ which may be a string containing xml, or an
        # IO object.
        def parse thing, &block
          if thing.respond_to?(:read) && thing.respond_to?(:close)
            parse_io(thing, &block)
          else
            parse_memory(thing, &block)
          end
        end

        ###
        # Parse given +io+
        def parse_io io, encoding = 'ASCII'
          @encoding = encoding
          push_parser = PushParser.new(@document, @filename)
          yield push_parser if block_given?
          _parse_io(push_parser, io)
        end

        ###
        # Parse a file with +filename+
        def parse_file filename
          raise ArgumentError unless filename
          raise Errno::ENOENT unless File.exists?(filename)
          raise Errno::EISDIR if File.directory?(filename)
          @filename = filename
          parse_io(File.open(filename, 'rb'))
        end

        def parse_memory data
          raise ArgumentError if data.nil?
          raise "data cannot be empty" if data.empty?
          push_parser = PushParser.new(@document)
          yield push_parser if block_given?
          push_parser.write(data, true) rescue nil
        end

        private

        def _parse_io(parser, io)
          while((data = io.read) && !data.empty?)
            parser.write(data) rescue nil
          end
          parser.finish rescue nil
        end
      end
    end
  end
end
