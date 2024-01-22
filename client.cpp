//
// Created by ismail on 13.09.21.
//
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <chrono>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#define BUF 4096
#define UDS_FILE "/tmp/sock.uds"


struct Bbox{
    int x1, y1, x2, y2;
};

struct Stone{
    Bbox bbox;
    int label;
    float score;
};

void sendImgSize(int socket, int imgSize) {
    std::string msg = "SIZE " + std::to_string(imgSize);
    const char *ch = msg.data();
    send(socket, ch, strlen(ch), 0);
}

void decodeDetectionResults(std::vector<std::vector<float>> &boxes,
                            std::vector<int> &labels,
                            std::vector<float> &scores,
                            std::string encodedString) {

    size_t pos;
    std::string categorySplit = "$";
    std::string category;
    std::vector<std::string> categories;
    while ((pos = encodedString.find(categorySplit)) != std::string::npos) {
        category = encodedString.substr(0, pos);
        categories.push_back(category);
        encodedString.erase(0, pos + categorySplit.length());
    }

    std::string entrySplit = "&";
    std::string bboxSplit = ",";
    std::string boxesStr = categories[0];
    std::string entry;
    while ((pos = boxesStr.find(entrySplit)) != std::string::npos) {
        entry = boxesStr.substr(0, pos);
        std::string bboxVal;
        std::vector<float> bboxEntry;
        size_t pos1;
        while ((pos1 = entry.find(bboxSplit)) != std::string::npos) {
            bboxVal = entry.substr(0, pos1);
            bboxEntry.push_back(std::stof(bboxVal));
            entry.erase(0, pos1 + bboxSplit.length());
        }
        boxes.push_back(bboxEntry);
        boxesStr.erase(0, pos + entrySplit.length());
    }

    std::string labelsStr = categories[1];
    while ((pos = labelsStr.find(entrySplit)) != std::string::npos) {
        entry = labelsStr.substr(0, pos);
        labels.push_back(std::stoi(entry));
        labelsStr.erase(0, pos + entrySplit.length());
    }

    std::string scoresStr = categories[2];
    while ((pos = scoresStr.find(entrySplit)) != std::string::npos) {
        entry = scoresStr.substr(0, pos);
        scores.push_back(std::stof(entry));
        scoresStr.erase(0, pos + entrySplit.length());
    }


}

int connectToPythonServer(){
    int create_socket;
    struct sockaddr_un address;
    if ((create_socket = socket(PF_LOCAL, SOCK_STREAM, 0)) > 0)
        printf("Socket wurde angelegt\n");
    address.sun_family = AF_LOCAL;
    strcpy(address.sun_path, UDS_FILE);
    if (connect(create_socket,
                (struct sockaddr *) &address,
                sizeof(address)) == 0)
        printf("Verbindung mit dem Server hergestellt\n");

    return create_socket;
}

std::string getEncodedDetectionResults(std::vector<std::vector<float>> &boxes,
                         std::vector<int> &labels,
                         std::vector<float> &scores,
                         const cv::Mat& image,
                         int socket){

    char *buffer = (char *) malloc(BUF);
    int size;

    // compress Mat->JPEG
    std::vector<uchar> buf;
    std::vector<int> param(2);
    param[0] = cv::IMWRITE_JPEG_QUALITY;
    param[1] = 95;
    cv::imencode(".jpg", image, buf, param);

    // send compressed image to server
    char *ch = reinterpret_cast<char *>(buf.data());

        sendImgSize(socket, buf.size());
        size = recv(socket, buffer, BUF - 1, 0);
        if (size > 0) {
            buffer[size] = '\0';
            if (strcmp(buffer, "GOT SIZE") == 0) {
                send(socket, ch, buf.size(), 0);
            }
        }
        size = recv(socket, buffer, BUF - 1, 0);
        if (size > 0) {
            buffer[size] = '\0';
        }

        return std::string(buffer);
}

std::vector<Stone> convertToStoneVec(std::vector<std::vector<float>> boxes,
                                     std::vector<int> labels,
                                     std::vector<float> scores){

    //assert(boxes.size() == labels.size() == scores.size());
    int size = boxes.size();
    std::vector<Stone> result(size);
    for(int i = 0; i < boxes.size(); i++){
        result[i].label = labels[i];
        result[i].score = scores[i];

        std::vector<float> box = boxes[i];
        result[i].bbox.x1 = box[0];
        result[i].bbox.y1 = box[1];
        result[i].bbox.x2 = box[2];
        result[i].bbox.y2 = box[3];
    }

    return result;
}

std::vector<Stone> detectStones(cv::Mat image){
    std::vector<std::vector<float>> boxes;
    std::vector<int> labels;
    std::vector<float> scores;

    auto start = std::chrono::high_resolution_clock::now();

    int socket = connectToPythonServer();
    std::string encodedRes = getEncodedDetectionResults(boxes, labels, scores, image, socket);
    decodeDetectionResults(boxes, labels, scores, encodedRes);
    std::vector<Stone> res = convertToStoneVec(boxes, labels, scores);
    close(socket);

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "Elapsed time: " << elapsed.count() << " s\n";

    return res;
}


int main(int argc, char **argv) {

    std::string image_path = "/path/to/image/x.jpg";
    cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);

    std::vector<Stone> detections = detectStones(img);
    for(auto d : detections){
        std::cout << d.bbox.x1 << " " << d.bbox.y1 << " " << d.bbox.x2 << " " << d.bbox.y2 << std::endl;
        std::cout << d.label << std::endl;
        std::cout << d.score << std::endl;
        std::cout << "\n";
    }

    return EXIT_SUCCESS;
}