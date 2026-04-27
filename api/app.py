from flask import Flask, jsonify, render_template
import os

app = Flask(__name__, template_folder='../templates')

@app.route('/')
def index():
    """Home page - game dashboard"""
    return render_template('index.html')

@app.route('/api/status')
def status():
    """API endpoint for game status"""
    return jsonify({
        'status': 'ok',
        'project': 'NovaStrike',
        'description': 'ESP32-S3 Arcade Space Shooter',
        'hardware': {
            'microcontroller': 'Seeed Xiao ESP32-S3',
            'display': 'SSD1306 0.96" OLED (128x64)',
            'audio': 'MAX98357A I2S Amplifier'
        }
    })

@app.route('/api/info')
def info():
    """Project information"""
    return jsonify({
        'name': 'NovaStrike',
        'version': '1.0.0',
        'description': 'Full-featured arcade space shooter for Xiao ESP32-S3 with OLED display and MAX98357A audio',
        'repository': 'https://github.com/Am4l-babu/ESP32S3-Novastrike',
        'license': 'MIT'
    })

if __name__ == '__main__':
    app.run(debug=True)
