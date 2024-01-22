import socket
import os
import cv2
import numpy as np
import stoneDetector as sd

if __name__ == '__main__':
    server_address = '/tmp/sock.uds'
    BUFFER = 1024

    # Make sure the socket does not already exist
    try:
        os.unlink(server_address)
    except OSError:
        if os.path.exists(server_address):
            raise

    # Create a UDS socket
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

    # Bind the socket to the port
    print('Starting up on %s' % server_address)
    sock.bind(server_address)

    # Listen for incoming connections
    sock.listen(1)

    # Load model and get ready for inferencing
    num_classes = 3
    path_to_trained_model = "/path/to/model/params/model.pth"
    try:
        model = sd.load_trained_model(num_classes, path_to_trained_model)
        model.eval()
        print("Model loaded and ready to inference!")
    except:
        raise Exception("Model could not be loaded! Check Path to model")

    # Wait for a connection
    while True:
        print('Waiting for a connection ...')
        connection, client_address = sock.accept()
        try:
            print('Connected!')
            # Receive the data in small chunks
            img = []
            print("Waiting for data ...")
            data = connection.recv(BUFFER)
            if data:
                txt = str(data, 'utf-8')
                if txt.startswith('SIZE'):
                    tmp = txt.split()
                    size = int(tmp[1])

                    msg = "GOT SIZE"
                    connection.sendall(msg.encode())

                    data = connection.recv(size)
                    print('Received %s bytes' % len(data))

            img = np.asarray(bytearray(data))
            opencv_mat = cv2.imdecode(img, cv2.IMREAD_COLOR)

            boxes, labels, scores = sd.inference_model(model, opencv_mat)
            #sd.draw_detections(boxes, labels, scores, opencv_mat, 0.6)

            detection_results = sd.encode_detection_results(boxes, labels, scores)
            connection.sendall(detection_results.encode())

        finally:
            # Clean up the connection
            connection.close()
