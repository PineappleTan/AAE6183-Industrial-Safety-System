import cv2
import time
from flask import Flask, Response
from ultralytics import YOLO
import paho.mqtt.client as mqtt
import threading

# Initialize Flask App for video streaming
app = Flask(__name__)

# MQTT Setup
MQTT_BROKER = "broker.hivemq.com"
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
client.connect(MQTT_BROKER, 1883, 60)

# Load YOLO Model
model = YOLO('fire.pt') 
cap = cv2.VideoCapture(0) # 0 is standard laptop webcam

def generate_frames():
    while cap.isOpened():
        start_time = time.time()
        ret, frame = cap.read()
        if not ret:
            break

        # Run YOLO Inference
        results = model(frame, verbose=False)
        fire_detected = False

        for result in results:
            boxes = result.boxes
            for box in boxes:
                class_id = int(box.cls[0])
                label = model.names[class_id]
                
                # If your specific model detects fire/smoke
                if label == 'fire': 
                    fire_detected = True
                    # Draw a bounding box on the frame to show on the UI
                    b = box.xyxy[0].to('cpu').detach().numpy().copy()
                    cv2.rectangle(frame, (int(b[0]), int(b[1])), (int(b[2]), int(b[3])), (0, 0, 255), 2)
                    cv2.putText(frame, "FIRE DETECTED", (int(b[0]), int(b[1])-10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

        # Measure inference latency for your trade-off analysis
        latency = (time.time() - start_time) * 1000 
        client.publish("room/analytics/latency", f"{latency:.2f}")

        # --- FIXED MQTT SAFETY DUAL-STATE PIPELINE ---
        if fire_detected:
            client.publish("room/safety/status", "FIRE_DETECTED")
        else:
            # Added: Constantly sends a safe heartbeat when no fire labels are present
            client.publish("room/safety/status", "SAFE")
        
        # Encode the frame as JPEG to send to HTML
        ret, buffer = cv2.imencode('.jpg', frame)
        frame_bytes = buffer.tobytes()
        
        # Yield the frame in a format the browser understands
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')

@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

def run_flask():
    app.run(host='0.0.0.0', port=5000, threaded=True, use_reloader=False)

if __name__ == "__main__":
    # Run Flask server in a secondary thread so it doesn't block processing loops
    flask_thread = threading.Thread(target=run_flask)
    flask_thread.daemon = True
    flask_thread.start()
    
    # Keeps MQTT background network thread loop alive
    client.loop_forever()