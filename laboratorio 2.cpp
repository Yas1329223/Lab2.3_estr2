#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <bitset>

using ordered_json = nlohmann::ordered_json;
using namespace std;

vector<ordered_json> inventory;

// Nodo para el árbol de Huffman
struct HuffmanNode {
    char ch;
    int freq;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(char ch, int freq) : ch(ch), freq(freq), left(nullptr), right(nullptr) {}
};

// Comparador para la cola de prioridad de Huffman
struct Compare {
    bool operator()(HuffmanNode* l, HuffmanNode* r) {
        return l->freq > r->freq;
    }
};

// Función para construir el árbol de Huffman
HuffmanNode* buildHuffmanTree(const unordered_map<char, int>& freqMap) {
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, Compare> pq;

    for (const auto& pair : freqMap) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    while (pq.size() != 1) {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();

        HuffmanNode* newNode = new HuffmanNode('\0', left->freq + right->freq);
        newNode->left = left;
        newNode->right = right;
        pq.push(newNode);
    }

    return pq.top();
}

// Función para generar los códigos de Huffman
void generateHuffmanCodes(HuffmanNode* root, string str, unordered_map<char, string>& huffmanCode) {
    if (!root) return;
    if (root->ch != '\0') {
        huffmanCode[root->ch] = str;
    }
    generateHuffmanCodes(root->left, str + "0", huffmanCode);
    generateHuffmanCodes(root->right, str + "1", huffmanCode);
}

// Codificación Huffman para un string
string huffmanCompress(const string& text) {
    unordered_map<char, int> freqMap;
    for (char ch : text) {
        freqMap[ch]++;
    }

    HuffmanNode* root = buildHuffmanTree(freqMap);

    unordered_map<char, string> huffmanCode;
    generateHuffmanCodes(root, "", huffmanCode);

    string compressed = "";
    for (char ch : text) {
        compressed += huffmanCode[ch];
    }

    return compressed;
}

// Compresión aritmética básica para un string
string arithmeticCompress(const string& text) {
    double low = 0.0, high = 1.0, range;

    for (char ch : text) {
        double charRange = 1.0 / 256;  // Suponemos distribución uniforme
        range = high - low;
        high = low + range * ((ch + 1) / 256.0);
        low = low + range * (ch / 256.0);
    }

    bitset<64> compressed(static_cast<long long>((low + high) / 2 * (1LL << 64)));
    return compressed.to_string();
}

void processInputFile(const string& inputFile) {
    ifstream file(inputFile);
    if (!file.is_open()) {
        cerr << "No se pudo abrir el archivo de entrada." << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        auto pos = line.find(';');
        if (pos == string::npos) continue;

        string operation = line.substr(0, pos);
        string data = line.substr(pos + 1);

        try {
            ordered_json item = ordered_json::parse(data);

            if (operation == "INSERT") {
                item["deleted"] = false;
                inventory.push_back(item);
            }
            else if (operation == "PATCH") {
                string isbn = item["isbn"].get<string>();
                for (auto& existingItem : inventory) {
                    if (existingItem["isbn"] == isbn && !existingItem["deleted"]) {
                        for (auto& el : item.items()) {
                            existingItem[el.key()] = el.value();
                        }
                        break;
                    }
                }
            }
            else if (operation == "DELETE") {
                string isbn = item["isbn"].get<string>();
                for (auto& existingItem : inventory) {
                    if (existingItem["isbn"] == isbn && !existingItem["deleted"]) {
                        existingItem["deleted"] = true;
                        break;
                    }
                }
            }
        }
        catch (const nlohmann::json::parse_error& e) {
            cerr << "Error de análisis JSON: " << e.what() << " en la línea: " << line << endl;
        }
        catch (const std::exception& e) {
            cerr << "Error: " << e.what() << " en la línea: " << line << endl;
        }
    }

    file.close();
}

void writeOutputFile(const string& outputFile, int& equal, int& decompress, int& huffman, int& arithmetic, int& either) {
    ofstream file(outputFile);
    if (!file.is_open()) {
        cerr << "No se pudo abrir el archivo de salida." << endl;
        return;
    }

    for (const auto& item : inventory) {
        if (!item["deleted"]) {
            ordered_json orderedItem;
            orderedItem["isbn"] = item["isbn"];
            orderedItem["name"] = item["name"];
            orderedItem["author"] = item["author"];
            orderedItem["category"] = item["category"];
            orderedItem["price"] = item["price"];
            orderedItem["quantity"] = item["quantity"];

            string name = item["name"];
            int nameSizeOriginal = static_cast<int>(name.size() * 8);  // Tamaño original en bits
            string huffmanCompressed = huffmanCompress(name);
            string arithmeticCompressed = arithmeticCompress(name);

            // Los tamaños de las cadenas comprimidas se calculan en bits
            int huffmanSize = huffmanCompressed.size();  // Tamaño en bits
            int arithmeticSize = arithmeticCompressed.size();  // Tamaño en bits

            orderedItem["namesize"] = nameSizeOriginal;
            orderedItem["namesizehuffman"] = huffmanSize;
            orderedItem["namesizearithmetic"] = arithmeticSize;

            // Contar los resultados de acuerdo a los criterios
            if (nameSizeOriginal == huffmanSize && nameSizeOriginal == arithmeticSize) {
                equal++;
            }
            else if (nameSizeOriginal < huffmanSize && nameSizeOriginal < arithmeticSize) {
                decompress++;
            }
            else if (huffmanSize < nameSizeOriginal && huffmanSize < arithmeticSize) {
                huffman++;
            }
            else if (arithmeticSize < nameSizeOriginal && arithmeticSize < huffmanSize) {
                arithmetic++;
            }
            // Caso para tamaños iguales entre Huffman y Aritmética
            else if (huffmanSize == arithmeticSize && huffmanSize != nameSizeOriginal) {
                either++;
            }

            file << orderedItem.dump() << endl;
        }
    }

    file.close();
}

void processSearchFile(const string& searchFile, const string& outputFile, const string& finalFile, int equal, int decompress, int huffman, int arithmetic, int either) {
    ifstream searchFileStream(searchFile);
    if (!searchFileStream.is_open()) {
        cerr << "No se pudo abrir el archivo de búsqueda." << endl;
        return;
    }

    ifstream outputFileStream(outputFile);
    if (!outputFileStream.is_open()) {
        cerr << "No se pudo abrir el archivo de salida para lectura." << endl;
        return;
    }

    vector<ordered_json> updatedInventory;
    string line;
    while (getline(outputFileStream, line)) {
        try {
            ordered_json item = ordered_json::parse(line);
            updatedInventory.push_back(item);
        }
        catch (const nlohmann::json::parse_error& e) {
            cerr << "Error de análisis JSON en el archivo de salida: " << e.what() << endl;
        }
        catch (const std::exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
    }

    outputFileStream.close();

    vector<ordered_json> searchResults;
    while (getline(searchFileStream, line)) {
        auto pos = line.find(';');
        if (pos == string::npos) continue;

        string searchType = line.substr(0, pos);
        string data = line.substr(pos + 1);

        try {
            ordered_json searchItem = ordered_json::parse(data);

            if (searchType == "SEARCH") {
                string name = searchItem["name"].get<string>();
                for (const auto& item : updatedInventory) {
                    if (item["name"].get<string>() == name) {
                        searchResults.push_back(item);
                    }
                }
            }
        }
        catch (const nlohmann::json::parse_error& e) {
            cerr << "Error de análisis JSON: " << e.what() << " en la línea: " << line << endl;
        }
        catch (const std::exception& e) {
            cerr << "Error: " << e.what() << " en la línea: " << line << endl;
        }
    }

    searchFileStream.close();

    ofstream finalFileStream(finalFile);
    if (!finalFileStream.is_open()) {
        cerr << "No se pudo abrir el archivo final." << endl;
        return;
    }

    // Escribir los resultados de búsqueda
    for (const auto& result : searchResults) {
        finalFileStream << result.dump() << endl;
    }

    // Escribir los resultados de compresión
    finalFileStream << "Resultados de Compresión:" << endl;
    finalFileStream << "Equal: " << equal << endl;
    finalFileStream << "Decompress: " << decompress << endl;
    finalFileStream << "Huffman: " << huffman << endl;
    finalFileStream << "Arithmetic: " << arithmetic << endl;

    finalFileStream.close();
}

int main() {
    string inputFile = "Ejemplo_lab01_books.csv";
    string searchFile = "Ejemplo_lab01_search.csv";
    string outputFile = "output.txt";
    string finalFile = "final.txt";

    int equal = 0, decompress = 0, huffman = 0, arithmetic = 0, either = 0;

    processInputFile(inputFile);
    writeOutputFile(outputFile, equal, decompress, huffman, arithmetic, either);
    processSearchFile(searchFile, outputFile, finalFile, equal, decompress, huffman, arithmetic, either);

    return 0;
}