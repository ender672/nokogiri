module Nokogiri
  module HTML
    module SAX
      ###
      # Context for HTML SAX parsers.  This class is usually not instantiated
      # by the user.  Instead, you should be looking at
      # Nokogiri::HTML::SAX::Parser
      class ParserContext < Nokogiri::XML::SAX::ParserContext
        def parse_with(parser)
          raise ArgumentError if parser.nil?

          @push_parser = Nokogiri::HTML::SAX::PushParser.new(parser.document, nil, @encoding)
          parse_with2
        end
      end
    end
  end
end
