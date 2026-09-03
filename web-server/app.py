from flask import (
    Flask,
    render_template,
    request,
    jsonify,
    send_from_directory
)

from datetime import datetime

import os
import pytz

app = Flask(__name__)

# Monitoring Data

duckweed_coverage = 0

logs = []

# Timezone


LOCAL_TZ = pytz.timezone(
    "Asia/Kuala_Lumpur"
)

# Image Upload Directory

UPLOAD_FOLDER = os.path.join(
    "static",
    "uploads"
)

os.makedirs(
    UPLOAD_FOLDER,
    exist_ok=True
)

app.config[
    "UPLOAD_FOLDER"
] = UPLOAD_FOLDER

# Local Timestamp

def get_local_timestamp():
    utc_time = datetime.utcnow()
    local_time = (
        utc_time
        .replace(tzinfo=pytz.utc)
        .astimezone(LOCAL_TZ)
    )
    return local_time.strftime(
        "%Y-%m-%d %H:%M:%S"
    )

# Dashboard

@app.route("/")
def dashboard():
    return render_template(
        "dashboard.html",
        coverage=duckweed_coverage,
        logs=logs
    )

# Coverage API

@app.route(
    "/update",
    methods=["GET", "POST"]
)
def update_coverage():
    global duckweed_coverage
    global logs

    # ESP32 sends new coverage
    if request.method == "POST":
        data = request.get_json(
            silent=True
        )

        if not data:
            return jsonify({
                "status": "error",
                "message": "Invalid JSON data"
            }), 400

        if "coverage" in data:
            try:
                coverage_value = float(
                    data["coverage"]
                )

                if (
                    0 <=
                    coverage_value <=
                    100
                ):
                    duckweed_coverage = (
                        coverage_value
                    )

                    timestamp = (
                        get_local_timestamp()
                    )

                    logs.append({
                        "timestamp":
                            timestamp,
                        "coverage":
                            duckweed_coverage
                    })

                    return jsonify({
                        "status":
                            "success",
                        "coverage":
                            duckweed_coverage,
                        "logs":
                            logs
                    }), 200


                return jsonify({
                    "status":
                        "error",
                    "message":
                        "Coverage must be between 0 and 100"
                }), 400

            except (
                ValueError,
                TypeError
            ):

                return jsonify({
                    "status":
                        "error",
                    "message":
                        "Invalid coverage value"
                }), 400


        return jsonify({
            "status":
                "error",
            "message":
                "Coverage value missing"
        }), 400

    # Browser/API GET request
    return jsonify({
        "status":
            "success",
        "coverage":
            duckweed_coverage,
        "logs":
            logs
    }), 200

# Image Upload

@app.route(
    "/upload",
    methods=["POST"]
)
def upload_image():
    if "image" not in request.files:
        return jsonify({
            "status":
                "error",
            "message":
                "No file part"
        }), 400

    file = request.files["image"]

    if file.filename == "":

        return jsonify({
            "status":
                "error",
            "message":
                "No selected file"
        }), 400

    file_path = os.path.join(
        app.config[
            "UPLOAD_FOLDER"
        ],
        "processed.jpg"
    )

    file.save(file_path)

    return jsonify({
        "status":
            "success",
        "message":
            "File uploaded successfully",
        "file_path":
            file_path
    }), 200

# Latest Image

@app.route("/image")
def get_image():
    return send_from_directory(
        app.config[
            "UPLOAD_FOLDER"
        ],
        "processed.jpg"

    )

# Development Server

if __name__ == "__main__":

    app.run(
        debug=True,
        host="0.0.0.0",
        port=5000
    )
