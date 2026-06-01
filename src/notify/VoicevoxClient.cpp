#include "notify/VoicevoxClient.hpp"

#include "httplib.h"
#include "util/Logger.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace aic {

namespace {

bool isAllowedLocalEndpoint(const std::string& endpoint) {
    std::string normalized = endpoint;
    while (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    return normalized == "http://127.0.0.1:50021";
}

std::string urlEncode(const std::string& text) {
    std::ostringstream out;
    const char* hex = "0123456789ABCDEF";
    for (unsigned char c : text) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out << (char)c;
        } else {
            out << '%' << hex[c >> 4] << hex[c & 15];
        }
    }
    return out.str();
}

} // namespace

bool VoicevoxClient::synthesizeToFile(const std::string& endpoint, int speaker,
                                      const std::string& text, const std::string& wavPath,
                                      std::string& resultMessage) {
    try {
        if (!isAllowedLocalEndpoint(endpoint)) {
            resultMessage = "VOICEVOX endpoint rejected (local endpoint only)";
            Logger::warn(resultMessage + ": " + endpoint);
            return false;
        }
        httplib::Client cli(endpoint);
        cli.set_connection_timeout(3, 0);
        cli.set_read_timeout(15, 0);
        cli.set_write_timeout(15, 0);

        const std::string queryPath = "/audio_query?text=" + urlEncode(text) +
                                      "&speaker=" + std::to_string(speaker);
        auto query = cli.Post(queryPath.c_str(), "", "application/json");
        if (!query) query = cli.Post(queryPath.c_str(), "", "application/json");
        if (!query || query->status != 200) {
            resultMessage = "VOICEVOX audio_query failed";
            Logger::warn(resultMessage);
            return false;
        }

        const std::string synthPath = "/synthesis?speaker=" + std::to_string(speaker);
        auto wav = cli.Post(synthPath.c_str(), query->body, "application/json");
        if (!wav) wav = cli.Post(synthPath.c_str(), query->body, "application/json");
        if (!wav || wav->status != 200 || wav->body.empty()) {
            resultMessage = "VOICEVOX synthesis failed";
            Logger::warn(resultMessage);
            return false;
        }

        const std::filesystem::path p(wavPath);
        if (p.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
        }
        std::ofstream out(wavPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            resultMessage = "VOICEVOX cache write failed";
            Logger::warn(resultMessage + ": " + wavPath);
            return false;
        }
        out.write(wav->body.data(), (std::streamsize)wav->body.size());
        resultMessage = "tts saved";
        return true;
    } catch (const std::exception& e) {
        resultMessage = std::string("VOICEVOX error: ") + e.what();
        Logger::warn(resultMessage);
        return false;
    } catch (...) {
        resultMessage = "VOICEVOX unknown error";
        Logger::warn(resultMessage);
        return false;
    }
}

} // namespace aic
