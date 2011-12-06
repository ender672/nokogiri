module Nokogiri
  module HTML
    ###
    # Nokogiri lets you write a SAX parser to process HTML but get HTML
    # correction features.
    #
    # See Nokogiri::HTML::SAX::Parser for a basic example of using a
    # SAX parser with HTML.
    #
    # For more information on SAX parsers, see Nokogiri::XML::SAX
    module SAX
      ###
      # This class lets you perform SAX style parsing on HTML with HTML
      # error correction.
      #
      # Here is a basic usage example:
      #
      #   class MyDoc < Nokogiri::XML::SAX::Document
      #     def start_element name, attributes = []
      #       puts "found a #{name}"
      #     end
      #   end
      #
      #   parser = Nokogiri::HTML::SAX::Parser.new(MyDoc.new)
      #   parser.parse(File.read(ARGV[0], 'rb'))
      #
      # For more information on SAX parsers, see Nokogiri::XML::SAX
      class Parser < Nokogiri::XML::SAX::Parser
        ###
        # Parse html stored in +data+ using +encoding+
        def initialize doc = Nokogiri::HTML::SAX::Document.new, encoding = 'UTF-8'
          super(doc, encoding)
        end

        ###
        # Parse given +io+
        def parse_io io, encoding = 'UTF-8'
          @encoding = encoding
          parser = get_parser
          yield parser if block_given?
          _parse_io(parser, io)
        end

        def parse_memory data, encoding = 'UTF-8'
          raise ArgumentError if data.nil?
          @encoding = encoding
          parser = get_parser
          yield parser if block_given?
          parser.write(data, true) rescue nil
        end

        private

        def get_parser
          enc_id = Nokogiri::XML::SAX::Parser::ENCODINGS[@encoding]
          native_parser = NativeParser.new(@document, @filename, enc_id)
          native_parser.force_encoding(enc_id) if enc_id
          native_parser
        end
      end
    end
  end
end
