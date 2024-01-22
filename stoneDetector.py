from torchvision.models.detection.faster_rcnn import FastRCNNPredictor
import torchvision
import torch
import torchvision.transforms as transforms
import cv2
import time


def load_trained_model(num_classes, path_to_model):
    # load an instance segmentation model pre-trained pre-trained on COCO
    model = torchvision.models.detection.fasterrcnn_resnet50_fpn(pretrained=True)
    # get number of input features for the classifier
    in_features = model.roi_heads.box_predictor.cls_score.in_features
    # replace the pre-trained head with a new one
    model.roi_heads.box_predictor = FastRCNNPredictor(in_features, num_classes)
    # load params of trained model
    model.load_state_dict(torch.load(path_to_model, map_location='cpu'))

    return model


def pre_process_image(img):
    """ Preprocess image and convert to tensor

    :arg img: opencv::Mat representing image

    :return unsqueezed tensor ready to feed to model
    """
    device = 'cpu'
    transform = transforms.Compose([transforms.ToTensor(), ])
    processed_img = transform(img).to(device)
    processed_img = processed_img.unsqueeze(0)

    return processed_img


# image = cv2.imread("/media/ismail/Samsung_T51/NMM_DATA/NMM_trainImages/stones_on_board/120.jpg")

def inference_model(model, img):
    """ Inference model on image

    :param model: model which should be inferences (Important: Model should be in eval() mode)
    :param img: Input image as opencv::Mat
    :return: Bounding-Boxes with labels and scores as seperate numpy arrays
    """
    print("Detection is running ...")
    start = time.time()
    img_tensor = pre_process_image(img)
    model_prediction = model(img_tensor)
    end = time.time()

    boxes = model_prediction[0]['boxes']
    labels = model_prediction[0]['labels']
    scores = model_prediction[0]['scores']

    boxes = boxes.detach().cpu().numpy()
    labels = labels.detach().cpu().numpy()
    scores = scores.detach().cpu().numpy()

    print("Inferencing model including preprocessing the image took %s s" % round((end - start), 4))
    print("")

    return boxes, labels, scores


def draw_detections(boxes, labels, scores, img, threshold):
    for i, bbox in enumerate(boxes):
        if scores[i] >= threshold:
            cv2.rectangle(img, (int(bbox[0]), int(bbox[1])), (int(bbox[2]), int(bbox[3])), (0, 255, 0), 2)
            cv2.putText(img, f'{labels[i]}', (int(bbox[0]), int(bbox[1] - 5)),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 1,
                        lineType=cv2.LINE_AA)

    cv2.imshow("Test", img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()


def encode_detection_results(boxes, labels, scores):
    boxes_str = ''
    labels_str = ''
    scores_str = ''
    for i, bbox in enumerate(boxes):
        boxes_str += str(bbox[0]) + ',' + str(bbox[1]) + ',' + str(bbox[2]) + ',' + str(bbox[3]) + ',&'
        labels_str += str(labels[i]) + '&'
        scores_str += str(scores[i]) + '&'

    return boxes_str + '$' + labels_str + '$' + scores_str + '$'
