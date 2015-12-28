

template <typename TextSource, typename TokenType, typename FormatterType>
void Highlight(TextSource* textSource,
               std::function<std::vector<TokenType>(TextSource*)> tokenizer,
               std::function<void(TextSource*), TokenType > highlighter,
               std::function<void(TextSource*), std::vector<TokenType> > merger,
               FormatterType format)
{
    std::vector<TokenType> tokens = tokenizer(textSource);
    for(TokenType& token : tokens)
    {
        token = highlighter(textSource, token, format);
    }
    merger(tokens);
}