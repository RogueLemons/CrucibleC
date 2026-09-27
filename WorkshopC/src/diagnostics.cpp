#include "diagnostics.hpp"

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <iostream>

namespace {

const char *levelName(RuleLevel level) {
    return level == RuleLevel::Warning ? "warning" : "error";
}

void writeTextLine(std::ostream &out, const DiagnosticRecord &record) {
    out << record.file << ":" << record.line << ":" << record.column
        << ":\t" << levelName(record.level) << ": " << record.message
        << " [" << codeInfo(record.code).id << "]\n";
}

// The same file can be reached through different spellings of its path,
// e.g. from different including files, so compare real paths
std::string canonicalPath(const std::string &file) {
    llvm::SmallString<256> real;

    if (!llvm::sys::fs::real_path(file, real))
        return std::string(real);

    return file;
}

std::string jsonString(const std::string &text) {
    std::string escaped = "\"";

    for (const unsigned char c : text) {
        switch (c) {
        case '"':  escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (c < 0x20) {
                static const char *hex = "0123456789abcdef";
                escaped += "\\u00";
                escaped += hex[c >> 4];
                escaped += hex[c & 0xf];
            }
            else {
                escaped += static_cast<char>(c);
            }
        }
    }

    return escaped + "\"";
}

// A file URI for SARIF, e.g. file:///C:/project/main.c
std::string fileUri(const std::string &file) {
    llvm::SmallString<256> path(canonicalPath(file));
    llvm::sys::path::native(path, llvm::sys::path::Style::posix);

    std::string uri = std::string(path);

    for (char &c : uri) {
        if (c == '\\')
            c = '/';
    }

    if (!uri.empty() && uri[0] == '/')
        return "file://" + uri;

    return "file:///" + uri;
}

} // namespace

void Diagnostics::setStreamText(bool enabled) {
    streamText = enabled;
}

void Diagnostics::report(
    RuleLevel level,
    DiagCode code,
    const clang::SourceManager &sm,
    clang::SourceLocation loc,
    const std::string &message
) {
    if (level == RuleLevel::Off)
        return;

    if (loc.isInvalid())
        return;

    clang::PresumedLoc presumed =
        sm.getPresumedLoc(loc);

    if (presumed.isInvalid())
        return;

    report(
        level,
        code,
        presumed.getFilename(),
        presumed.getLine(),
        presumed.getColumn(),
        message
    );
}

void Diagnostics::report(
    RuleLevel level,
    DiagCode code,
    const std::string &file,
    unsigned line,
    unsigned column,
    const std::string &message
) {
    if (level == RuleLevel::Off)
        return;

    const auto key = std::make_tuple(
        canonicalPath(file), line, column, static_cast<int>(level),
        static_cast<int>(code), message);

    if (!seen.insert(key).second)
        return;

    DiagnosticRecord record{file, line, column, level, code, message};

    if (streamText)
        writeTextLine(std::cerr, record);

    records.push_back(std::move(record));

    if (level == RuleLevel::Warning)
        warnings++;
    else if (level == RuleLevel::Error)
        errors++;
}

int Diagnostics::getWarnings() const {
    return warnings;
}

int Diagnostics::getErrors() const {
    return errors;
}

void Diagnostics::writeText(std::ostream &out) const {
    for (const auto &record : records)
        writeTextLine(out, record);
}

void Diagnostics::writeJson(std::ostream &out) const {
    out << "{\n";
    out << "  \"warnings\": " << warnings << ",\n";
    out << "  \"errors\": " << errors << ",\n";
    out << "  \"diagnostics\": [";

    for (size_t i = 0; i < records.size(); ++i) {
        const auto &record = records[i];

        out << (i == 0 ? "\n" : ",\n");
        out << "    {"
            << "\"file\": " << jsonString(record.file) << ", "
            << "\"line\": " << record.line << ", "
            << "\"column\": " << record.column << ", "
            << "\"level\": " << jsonString(levelName(record.level)) << ", "
            << "\"code\": " << jsonString(codeInfo(record.code).id) << ", "
            << "\"name\": " << jsonString(codeInfo(record.code).name) << ", "
            << "\"message\": " << jsonString(record.message)
            << "}";
    }

    out << (records.empty() ? "]\n" : "\n  ]\n");
    out << "}\n";
}

void Diagnostics::writeSarif(std::ostream &out) const {
    out << "{\n";
    out << "  \"$schema\": \"https://json.schemastore.org/sarif-2.1.0.json\",\n";
    out << "  \"version\": \"2.1.0\",\n";
    out << "  \"runs\": [\n";
    out << "    {\n";
    out << "      \"tool\": {\n";
    out << "        \"driver\": {\n";
    out << "          \"name\": \"WorkshopC\",\n";
    out << "          \"rules\": [";

    // Every code, so that viewers can describe each result's ruleId
    for (size_t i = 0; i < kDiagnosticCodeCount; ++i) {
        const auto &info = kDiagnosticCodes[i];

        out << (i == 0 ? "\n" : ",\n");
        out << "            {"
            << "\"id\": " << jsonString(info.id) << ", "
            << "\"name\": " << jsonString(info.name) << ", "
            << "\"shortDescription\": { \"text\": " << jsonString(info.description) << " }"
            << "}";
    }

    out << "\n          ]\n";
    out << "        }\n";
    out << "      },\n";
    out << "      \"results\": [";

    for (size_t i = 0; i < records.size(); ++i) {
        const auto &record = records[i];

        out << (i == 0 ? "\n" : ",\n");
        out << "        {"
            << "\"ruleId\": " << jsonString(codeInfo(record.code).id) << ", "
            << "\"ruleIndex\": " << static_cast<size_t>(record.code) << ", "
            << "\"level\": " << jsonString(levelName(record.level)) << ", "
            << "\"message\": { \"text\": " << jsonString(record.message) << " }, "
            << "\"locations\": [ { \"physicalLocation\": { "
            << "\"artifactLocation\": { \"uri\": " << jsonString(fileUri(record.file)) << " }, "
            << "\"region\": { \"startLine\": " << record.line
            << ", \"startColumn\": " << record.column << " } } } ]"
            << "}";
    }

    out << (records.empty() ? "]\n" : "\n      ]\n");
    out << "    }\n";
    out << "  ]\n";
    out << "}\n";
}
