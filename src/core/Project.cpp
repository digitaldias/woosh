/**
 * @file Project.cpp
 * @brief Implementation of Project class with JSON serialization.
 */

#include "Project.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// Simple JSON writing helpers (avoiding external dependencies)
namespace {

std::string escapeJson(const std::string& s) {
    std::string result;
    result.reserve(s.size() + 10);
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    return result;
}

std::string indent(int level) {
    return std::string(level * 2, ' ');
}

// Simple JSON parser helpers
class JsonValue;

enum class JsonType { Null, Bool, Number, String, Array, Object };

class JsonValue {
public:
    JsonType type{JsonType::Null};
    bool boolValue{false};
    double numberValue{0.0};
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::vector<std::pair<std::string, JsonValue>> objectValue;
    
    [[nodiscard]] bool isObject() const { return type == JsonType::Object; }
    [[nodiscard]] bool isArray() const { return type == JsonType::Array; }
    [[nodiscard]] bool isString() const { return type == JsonType::String; }
    [[nodiscard]] bool isNumber() const { return type == JsonType::Number; }
    [[nodiscard]] bool isBool() const { return type == JsonType::Bool; }
    
    [[nodiscard]] const JsonValue* get(const std::string& key) const {
        if (type != JsonType::Object) return nullptr;
        for (const auto& [k, v] : objectValue) {
            if (k == key) return &v;
        }
        return nullptr;
    }
    
    [[nodiscard]] std::string getString(const std::string& key, const std::string& def = "") const {
        auto* v = get(key);
        return (v && v->isString()) ? v->stringValue : def;
    }
    
    [[nodiscard]] double getNumber(const std::string& key, double def = 0.0) const {
        auto* v = get(key);
        return (v && v->isNumber()) ? v->numberValue : def;
    }
    
    [[nodiscard]] int getInt(const std::string& key, int def = 0) const {
        return static_cast<int>(getNumber(key, static_cast<double>(def)));
    }
    
    [[nodiscard]] bool getBool(const std::string& key, bool def = false) const {
        auto* v = get(key);
        return (v && v->isBool()) ? v->boolValue : def;
    }
    
    [[nodiscard]] const JsonValue* getArray(const std::string& key) const {
        auto* v = get(key);
        return (v && v->isArray()) ? v : nullptr;
    }
    
    [[nodiscard]] const JsonValue* getObject(const std::string& key) const {
        auto* v = get(key);
        return (v && v->isObject()) ? v : nullptr;
    }
};

class JsonParser {
public:
    explicit JsonParser(const std::string& json) : json_(json), pos_(0) {}
    
    [[nodiscard]] std::optional<JsonValue> parse() {
        skipWhitespace();
        if (pos_ >= json_.size()) return std::nullopt;
        return parseValue();
    }
    
private:
    const std::string& json_;
    size_t pos_;
    
    void skipWhitespace() {
        while (pos_ < json_.size() && std::isspace(json_[pos_])) ++pos_;
    }
    
    [[nodiscard]] char peek() const {
        return pos_ < json_.size() ? json_[pos_] : '\0';
    }
    
    char consume() {
        return pos_ < json_.size() ? json_[pos_++] : '\0';
    }
    
    bool consume(char expected) {
        skipWhitespace();
        if (peek() == expected) {
            ++pos_;
            return true;
        }
        return false;
    }
    
    [[nodiscard]] std::optional<JsonValue> parseValue() {
        skipWhitespace();
        char c = peek();
        
        if (c == '"') return parseString();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == 't' || c == 'f') return parseBool();
        if (c == 'n') return parseNull();
        if (c == '-' || std::isdigit(c)) return parseNumber();
        
        return std::nullopt;
    }
    
    [[nodiscard]] std::optional<JsonValue> parseString() {
        if (!consume('"')) return std::nullopt;
        
        std::string result;
        while (pos_ < json_.size() && json_[pos_] != '"') {
            if (json_[pos_] == '\\' && pos_ + 1 < json_.size()) {
                ++pos_;
                switch (json_[pos_]) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    default:   result += json_[pos_]; break;
                }
            } else {
                result += json_[pos_];
            }
            ++pos_;
        }
        
        if (!consume('"')) return std::nullopt;
        
        JsonValue v;
        v.type = JsonType::String;
        v.stringValue = std::move(result);
        return v;
    }
    
    [[nodiscard]] std::optional<JsonValue> parseNumber() {
        size_t start = pos_;
        if (peek() == '-') ++pos_;
        while (pos_ < json_.size() && std::isdigit(json_[pos_])) ++pos_;
        if (pos_ < json_.size() && json_[pos_] == '.') {
            ++pos_;
            while (pos_ < json_.size() && std::isdigit(json_[pos_])) ++pos_;
        }
        if (pos_ < json_.size() && (json_[pos_] == 'e' || json_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < json_.size() && (json_[pos_] == '+' || json_[pos_] == '-')) ++pos_;
            while (pos_ < json_.size() && std::isdigit(json_[pos_])) ++pos_;
        }
        
        JsonValue v;
        v.type = JsonType::Number;
        v.numberValue = std::stod(json_.substr(start, pos_ - start));
        return v;
    }
    
    [[nodiscard]] std::optional<JsonValue> parseBool() {
        JsonValue v;
        v.type = JsonType::Bool;
        
        if (json_.substr(pos_, 4) == "true") {
            pos_ += 4;
            v.boolValue = true;
            return v;
        }
        if (json_.substr(pos_, 5) == "false") {
            pos_ += 5;
            v.boolValue = false;
            return v;
        }
        return std::nullopt;
    }
    
    [[nodiscard]] std::optional<JsonValue> parseNull() {
        if (json_.substr(pos_, 4) == "null") {
            pos_ += 4;
            JsonValue v;
            v.type = JsonType::Null;
            return v;
        }
        return std::nullopt;
    }
    
    [[nodiscard]] std::optional<JsonValue> parseArray() {
        if (!consume('[')) return std::nullopt;
        
        JsonValue v;
        v.type = JsonType::Array;
        
        skipWhitespace();
        if (peek() == ']') {
            ++pos_;
            return v;
        }
        
        while (true) {
            auto elem = parseValue();
            if (!elem) return std::nullopt;
            v.arrayValue.push_back(std::move(*elem));
            
            skipWhitespace();
            if (peek() == ']') {
                ++pos_;
                return v;
            }
            if (!consume(',')) return std::nullopt;
        }
    }
    
    [[nodiscard]] std::optional<JsonValue> parseObject() {
        if (!consume('{')) return std::nullopt;
        
        JsonValue v;
        v.type = JsonType::Object;
        
        skipWhitespace();
        if (peek() == '}') {
            ++pos_;
            return v;
        }
        
        while (true) {
            auto key = parseString();
            if (!key || !key->isString()) return std::nullopt;
            
            skipWhitespace();
            if (!consume(':')) return std::nullopt;
            
            auto value = parseValue();
            if (!value) return std::nullopt;
            
            v.objectValue.emplace_back(key->stringValue, std::move(*value));
            
            skipWhitespace();
            if (peek() == '}') {
                ++pos_;
                return v;
            }
            if (!consume(',')) return std::nullopt;
        }
    }
};

} // anonymous namespace

// ============================================================================
// Project implementation
// ============================================================================

void Project::setName(const std::string& name) {
    if (name_ != name) {
        name_ = name;
        dirty_ = true;
    }
}

void Project::setRawFolder(const std::string& path) {
    if (rawFolder_ != path) {
        rawFolder_ = path;
        dirty_ = true;
    }
}

void Project::setGameFolder(const std::string& path) {
    if (gameFolder_ != path) {
        gameFolder_ = path;
        dirty_ = true;
    }
}

void Project::setExportSettings(const ExportSettings& settings) {
    exportSettings_ = settings;
    dirty_ = true;
}

void Project::setProcessingSettings(const ProcessingSettings& settings) {
    processingSettings_ = settings;
    dirty_ = true;
}

void Project::addClipState(const ClipState& state) {
    clipStates_.push_back(state);
    dirty_ = true;
}

void Project::updateClipState(const std::string& relativePath,
                              const std::function<void(ClipState&)>& updater) {
    for (auto& state : clipStates_) {
        if (state.relativePath == relativePath) {
            updater(state);
            dirty_ = true;
            return;
        }
    }
}

ClipState* Project::findClipState(const std::string& relativePath) {
    for (auto& state : clipStates_) {
        if (state.relativePath == relativePath) {
            return &state;
        }
    }
    return nullptr;
}

const ClipState* Project::findClipState(const std::string& relativePath) const {
    for (const auto& state : clipStates_) {
        if (state.relativePath == relativePath) {
            return &state;
        }
    }
    return nullptr;
}

bool Project::removeClipState(const std::string& relativePath) {
    auto it = std::find_if(clipStates_.begin(), clipStates_.end(),
        [&](const ClipState& s) { return s.relativePath == relativePath; });
    
    if (it != clipStates_.end()) {
        clipStates_.erase(it);
        dirty_ = true;
        return true;
    }
    return false;
}

void Project::clearClipStates() {
    if (!clipStates_.empty()) {
        clipStates_.clear();
        dirty_ = true;
    }
}

bool Project::save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    // Write JSON manually for simplicity
    file << "{\n";
    file << indent(1) << "\"version\": " << kCurrentVersion << ",\n";
    file << indent(1) << "\"name\": \"" << escapeJson(name_) << "\",\n";
    file << indent(1) << "\"rawFolder\": \"" << escapeJson(rawFolder_) << "\",\n";
    file << indent(1) << "\"gameFolder\": \"" << escapeJson(gameFolder_) << "\",\n";
    
    // Export settings
    file << indent(1) << "\"exportSettings\": {\n";
    file << indent(2) << "\"format\": \"" << (exportSettings_.format == ExportFormat::MP3 ? "mp3" : 
                                               exportSettings_.format == ExportFormat::OGG ? "ogg" : "wav") << "\",\n";
    file << indent(2) << "\"mp3Bitrate\": " << exportSettings_.mp3Bitrate << ",\n";
    file << indent(2) << "\"gameName\": \"" << escapeJson(exportSettings_.gameName) << "\",\n";
    file << indent(2) << "\"authorName\": \"" << escapeJson(exportSettings_.authorName) << "\",\n";
    file << indent(2) << "\"embedMetadata\": " << (exportSettings_.embedMetadata ? "true" : "false") << "\n";
    file << indent(1) << "},\n";
    
    // Processing settings
    file << indent(1) << "\"processingSettings\": {\n";
    file << indent(2) << "\"normalizeTargetDb\": " << processingSettings_.normalizeTargetDb << ",\n";
    file << indent(2) << "\"compThreshold\": " << processingSettings_.compThreshold << ",\n";
    file << indent(2) << "\"compRatio\": " << processingSettings_.compRatio << ",\n";
    file << indent(2) << "\"compAttackMs\": " << processingSettings_.compAttackMs << ",\n";
    file << indent(2) << "\"compReleaseMs\": " << processingSettings_.compReleaseMs << ",\n";
    file << indent(2) << "\"compMakeupDb\": " << processingSettings_.compMakeupDb << "\n";
    file << indent(1) << "},\n";
    
    // Clip states
    file << indent(1) << "\"clips\": [\n";
    for (size_t i = 0; i < clipStates_.size(); ++i) {
        const auto& clip = clipStates_[i];
        file << indent(2) << "{\n";
        file << indent(3) << "\"relativePath\": \"" << escapeJson(clip.relativePath) << "\",\n";
        file << indent(3) << "\"isNormalized\": " << (clip.isNormalized ? "true" : "false") << ",\n";
        file << indent(3) << "\"isCompressed\": " << (clip.isCompressed ? "true" : "false") << ",\n";
        file << indent(3) << "\"isTrimmed\": " << (clip.isTrimmed ? "true" : "false") << ",\n";
        file << indent(3) << "\"isExported\": " << (clip.isExported ? "true" : "false") << ",\n";
        file << indent(3) << "\"normalizeTargetDb\": " << clip.normalizeTargetDb << ",\n";
        file << indent(3) << "\"compressor\": {\n";
        file << indent(4) << "\"threshold\": " << clip.compressorSettings.threshold << ",\n";
        file << indent(4) << "\"ratio\": " << clip.compressorSettings.ratio << ",\n";
        file << indent(4) << "\"attackMs\": " << clip.compressorSettings.attackMs << ",\n";
        file << indent(4) << "\"releaseMs\": " << clip.compressorSettings.releaseMs << ",\n";
        file << indent(4) << "\"makeupDb\": " << clip.compressorSettings.makeupDb << "\n";
        file << indent(3) << "},\n";
        file << indent(3) << "\"trimStartSec\": " << clip.trimStartSec << ",\n";
        file << indent(3) << "\"trimEndSec\": " << clip.trimEndSec << ",\n";
        file << indent(3) << "\"exportedFilename\": \"" << escapeJson(clip.exportedFilename) << "\"\n";
        file << indent(2) << "}" << (i < clipStates_.size() - 1 ? "," : "") << "\n";
    }
    file << indent(1) << "]\n";
    
    file << "}\n";
    
    file.close();
    
    if (file.good()) {
        filePath_ = path;
        dirty_ = false;
        return true;
    }
    return false;
}

std::optional<Project> Project::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    JsonParser parser(content);
    auto root = parser.parse();
    if (!root || !root->isObject()) return std::nullopt;
    
    Project project;
    project.filePath_ = path;
    
    // Basic properties
    project.name_ = root->getString("name");
    project.rawFolder_ = root->getString("rawFolder");
    project.gameFolder_ = root->getString("gameFolder");
    
    // Export settings
    if (auto* exp = root->getObject("exportSettings")) {
        std::string fmt = exp->getString("format", "mp3");
        if (fmt == "ogg") project.exportSettings_.format = ExportFormat::OGG;
        else if (fmt == "wav") project.exportSettings_.format = ExportFormat::WAV;
        else project.exportSettings_.format = ExportFormat::MP3;
        
        project.exportSettings_.mp3Bitrate = exp->getInt("mp3Bitrate", 192);
        project.exportSettings_.gameName = exp->getString("gameName");
        project.exportSettings_.authorName = exp->getString("authorName");
        project.exportSettings_.embedMetadata = exp->getBool("embedMetadata", true);
    }
    
    // Processing settings
    if (auto* proc = root->getObject("processingSettings")) {
        project.processingSettings_.normalizeTargetDb = proc->getNumber("normalizeTargetDb", -1.0);
        project.processingSettings_.compThreshold = static_cast<float>(proc->getNumber("compThreshold", -12.0));
        project.processingSettings_.compRatio = static_cast<float>(proc->getNumber("compRatio", 4.0));
        project.processingSettings_.compAttackMs = static_cast<float>(proc->getNumber("compAttackMs", 10.0));
        project.processingSettings_.compReleaseMs = static_cast<float>(proc->getNumber("compReleaseMs", 100.0));
        project.processingSettings_.compMakeupDb = static_cast<float>(proc->getNumber("compMakeupDb", 0.0));
    }
    
    // Clip states
    if (auto* clips = root->getArray("clips")) {
        for (const auto& clipJson : clips->arrayValue) {
            if (!clipJson.isObject()) continue;
            
            ClipState state;
            state.relativePath = clipJson.getString("relativePath");
            state.isNormalized = clipJson.getBool("isNormalized");
            state.isCompressed = clipJson.getBool("isCompressed");
            state.isTrimmed = clipJson.getBool("isTrimmed");
            state.isExported = clipJson.getBool("isExported");
            state.normalizeTargetDb = clipJson.getNumber("normalizeTargetDb");
            state.trimStartSec = clipJson.getNumber("trimStartSec");
            state.trimEndSec = clipJson.getNumber("trimEndSec");
            state.exportedFilename = clipJson.getString("exportedFilename");
            
            if (auto* comp = clipJson.getObject("compressor")) {
                state.compressorSettings.threshold = static_cast<float>(comp->getNumber("threshold"));
                state.compressorSettings.ratio = static_cast<float>(comp->getNumber("ratio", 1.0));
                state.compressorSettings.attackMs = static_cast<float>(comp->getNumber("attackMs", 10.0));
                state.compressorSettings.releaseMs = static_cast<float>(comp->getNumber("releaseMs", 100.0));
                state.compressorSettings.makeupDb = static_cast<float>(comp->getNumber("makeupDb"));
            }
            
            project.clipStates_.push_back(std::move(state));
        }
    }
    
    project.dirty_ = false;
    return project;
}
