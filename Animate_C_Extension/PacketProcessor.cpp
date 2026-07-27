#include "PacketProcessor.h"
#include "myUtil.h"
#include <ranges>

PacketProcessor& PacketProcessor::GetInstance() {
    static PacketProcessor instance;
    return instance;
}

PacketProcessor::PacketProcessor() {}

void PacketProcessor::AppendData(const std::string& data) {
    buffer.append(data);
    parse();
}

void PacketProcessor::parse() {
    std::string line = buffer;
    buffer.clear();

    std::vector<std::string_view> colonSplit = split(line, ':');
    if (colonSplit.size() < 2) return;

    const std::string jsflName(colonSplit[0]);
    std::string_view projectNames = colonSplit[1];

    if (!projectNames.empty()) {
        auto view = split(projectNames, ',') | std::views::transform([](std::string_view sv) {
            return std::string(sv);
            });

        auto& entry = parsedData[jsflName];
        entry.insert(entry.end(), view.begin(), view.end());
    }
}

const std::vector<std::string>& PacketProcessor::GetParsedDataByName(const std::wstring& wsv) {
    static const std::vector<std::string> empty;
    std::string str(wsv.begin(), wsv.end());
    auto it = parsedData.find(str);
    return (it != parsedData.end()) ? it->second : empty;
}

const std::vector<std::string>& PacketProcessor::GetParsedDataByName(const std::string& sv) {
    static const std::vector<std::string> empty;
    auto it = parsedData.find(sv);
    return (it != parsedData.end()) ? it->second : empty;
}
