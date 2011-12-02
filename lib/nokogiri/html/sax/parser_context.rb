module Nokogiri
  module HTML
    module SAX
      ###
      # Context for HTML SAX parsers.  This class is usually not instantiated
      # by the user.  Instead, you should be looking at
      # Nokogiri::HTML::SAX::Parser
      class ParserContext < Nokogiri::XML::SAX::ParserContext
      end
    end
  end
end
