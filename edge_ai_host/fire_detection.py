import cv2
import time
from ultralytics import YOLO
import paho.mqtt.client as mqtt

# MQTT Setup
MQTT_BROKER = "broker.hivemq.com"
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
client.connect(MQTT_BROKER, 1883, 60)

# Load a pre-trained fire detection model or custom weight file
# Note: You can replace 'yolov8n.pt' with a fire-tuned weight 'fire.pt'
model = YOLO('yolov8n.pt') 

cap = cv2.VideoCapture(0) # 0 is standard laptop webcam

while cap.isOpened():
    start_time = time.time()
    ret, frame = cap.read()
    if not ret:
        break

    # Run inference
    results = model(frame, verbose=False)
    fire_detected = False

    for result in results:
        boxes = result.boxes
        for box in boxes:
            class_id = int(box.cls[0])
            label = model.names[class_id]
            
            # Assuming your model class label includes 'fire' or 'smoke'
            if label == 'fire': 
                fire_detected = True
                
    # Trade-off analysis metric: Calculate latency
    latency = (time.time() - start_time) * 1000 
    client.publish("room/analytics/latency", f"{latency:.2f}")

    if fire_detected:
        client.publish("room/safety/status", "FIRE_DETECTED")
    
    # Display feed
    cv2.imshow("Edge AI Vision Feed", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()