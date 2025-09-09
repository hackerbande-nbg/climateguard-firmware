import os
from dotenv import load_dotenv
import qrcode

# Load .env from parent directory
dotenv_path = os.path.join(os.path.dirname(__file__), '.', '.env')
load_dotenv(dotenv_path)

url = os.getenv("QR_URL")
if not url:
    print("QR_URL not set in .env file.")
    exit(1)

img = qrcode.make(url)
img.save("qrcode.png")
print("QR code saved as qrcode.png")