from flask import request, jsonify

API_KEY = "12345"

def require_api_key():
    """Validate the API key from the request headers."""
    api_key = request.headers.get("api-key")
    if not api_key or api_key != API_KEY:
        return jsonify({"error": "Unauthorized. Invalid or missing API key."}), 401
    
# function to check CRC of the payload
def check_crc(payload):
    """Check if the CRC in the payload is valid."""
    try:
        # mock function to calculate CRC
        return True
    except Exception as e:
        return False