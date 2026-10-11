#pragma once

#include <cctype>
#include <regex>
#include <sstream>
#include <string>

namespace glweb
{
    inline bool IsIdentifierCharacter(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    inline std::string TrimWhitespace(std::string const & text)
    {
        size_t begin = 0;
        while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])))
        {
            ++begin;
        }

        size_t end = text.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
        {
            --end;
        }

        return text.substr(begin, end - begin);
    }

    inline std::string ReplaceIdentifier(
        std::string const & text,
        std::string const & from,
        std::string const & to)
    {
        std::string result;
        size_t position = 0;

        while (true)
        {
            size_t const found = text.find(from, position);
            if (found == std::string::npos)
            {
                result.append(text, position, std::string::npos);
                break;
            }

            size_t const end = found + from.size();
            bool const startsAtBoundary = found == 0 || !IsIdentifierCharacter(text[found - 1]);
            bool const endsAtBoundary = end >= text.size() || !IsIdentifierCharacter(text[end]);

            result.append(text, position, found - position);
            result.append((startsAtBoundary && endsAtBoundary) ? to : from);
            position = end;
        }

        return result;
    }

    inline std::string ConvertTexture1DCalls(std::string const & text)
    {
        std::string const name = "texture1D";

        std::string result;
        size_t position = 0;

        while (true)
        {
            size_t const found = text.find(name, position);
            if (found == std::string::npos)
            {
                result.append(text, position, std::string::npos);
                return result;
            }

            size_t open = found + name.size();
            while (open < text.size() && std::isspace(static_cast<unsigned char>(text[open])))
            {
                ++open;
            }

            size_t close = std::string::npos;
            size_t comma = std::string::npos;

            if (open < text.size() && text[open] == '(')
            {
                int depth = 0;
                for (size_t i = open; i < text.size(); ++i)
                {
                    char const c = text[i];
                    if (c == '(')
                    {
                        ++depth;
                    }
                    else if (c == ')')
                    {
                        --depth;
                        if (depth == 0)
                        {
                            close = i;
                            break;
                        }
                    }
                    else if (c == ',' && depth == 1 && comma == std::string::npos)
                    {
                        comma = i;
                    }
                }
            }

            if (close == std::string::npos || comma == std::string::npos)
            {
                result.append(text, position, found + name.size() - position);
                position = found + name.size();
                continue;
            }

            result.append(text, position, found - position);
            result += "texture(";
            result.append(text, open + 1, comma - open - 1);
            result += ", vec2(";
            result.append(text, comma + 1, close - comma - 1);
            result += ", 0.5))";
            position = close + 1;
        }
    }

    inline std::string InjectPointSizeAssignment(std::string const & text)
    {
        static std::regex const mainRegex(R"!(void\s+main\s*\(\s*(void)?\s*\)\s*\{)!");

        std::smatch match;
        if (!std::regex_search(text, match, mainRegex))
        {
            return text;
        }

        size_t const insertionPoint = static_cast<size_t>(match.position(0) + match.length(0));

        std::string result = text.substr(0, insertionPoint);
        result += "\n    gl_PointSize = glwebPointSize > 0.0 ? glwebPointSize : 1.0;\n";
        result += text.substr(insertionPoint);
        return result;
    }

    inline std::string ConvertShaderToEs300(
        std::string const & source,
        bool isVertexShader)
    {
        std::istringstream stream(source);
        std::string body;
        std::string line;
        bool isVersionSkipped = false;

        while (std::getline(stream, line))
        {
            std::string const trimmed = TrimWhitespace(line);

            if (!isVersionSkipped && trimmed.rfind("#version", 0) == 0)
            {
                isVersionSkipped = true;
                continue;
            }

            if (trimmed == "#define in attribute"
                || trimmed == "#define in varying"
                || trimmed == "#define out varying")
            {
                continue;
            }

            body += line;
            body += '\n';
        }

        body = ConvertTexture1DCalls(body);
        body = ReplaceIdentifier(body, "texture2D", "texture");
        body = ReplaceIdentifier(body, "sampler1D", "sampler2D");

        std::string header = "#version 300 es\n";
        header += "precision highp float;\n";
        header += "precision highp int;\n";
        header += "precision highp sampler2D;\n";

        if (isVertexShader)
        {
            header += "uniform float glwebPointSize;\n";
            body = InjectPointSizeAssignment(body);
        }
        else
        {
            std::string const converted = ReplaceIdentifier(body, "gl_FragColor", "glwebFragColor");
            if (converted != body)
            {
                header += "layout(location = 0) out vec4 glwebFragColor;\n";
                body = converted;
            }
        }

        return header + body;
    }
}
