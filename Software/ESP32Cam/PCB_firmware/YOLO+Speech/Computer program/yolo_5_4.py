import cv2
import torch
import time
import threading
from queue import Queue
from gtts import gTTS
import os
from playsound import playsound


speech_queue = Queue()

def speech_worker():
    while True:
        text = speech_queue.get()

        # Generate temporary audio file
        tts = gTTS(text=text, lang='en')
        filename = "tts_output.mp3"
        tts.save(filename)

        # Play it
        playsound(filename)

        # Remove file
        os.remove(filename)

        speech_queue.task_done()

threading.Thread(target=speech_worker, daemon=True).start()

def speak_objects(objects):
    text = " ".join(objects)
    speech_queue.put(text)


cap = cv2.VideoCapture("http://10.54.138.197:81/stream")

if not cap.isOpened():
    print("Failed to open camera")
    exit()


device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = torch.hub.load('ultralytics/yolov5', 'yolov5s', pretrained=True)
model = model.to(device)
model.eval()


last_spoken_objects = set()
speak_delay = 1.0
last_speak_time = time.time()


while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to capture frame")
        break

    start = time.time()

    img = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = model(img, size=640)

    annotated = results.render()[0]
    annotated = cv2.cvtColor(annotated, cv2.COLOR_RGB2BGR)

    fps = 1 / (time.time() - start)
    cv2.putText(annotated, f"FPS: {fps:.2f}", (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

    cv2.imshow("YOLOv5 Detection", annotated)

    # Extract objects
    df = results.pandas().xyxy[0]
    current_objects = set(df['name'].tolist())

    # Speak new objects
    now = time.time()
    if now - last_speak_time > speak_delay:
        new_objects = current_objects - last_spoken_objects
        if new_objects:
            speak_objects(new_objects)
            last_spoken_objects = current_objects
            last_speak_time = now

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()