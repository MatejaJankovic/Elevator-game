# Script to generate status text textures
# Run with: python generate_status_textures.py
# Requires: pip install Pillow

from PIL import Image, ImageDraw, ImageFont
import os

def create_text_image(text, filename, width=600, height=100, bg_color=(0, 0, 0, 0), text_color=(0, 0, 0, 255)):
    """Create a PNG image with text"""
    img = Image.new('RGBA', (width, height), bg_color)
    draw = ImageDraw.Draw(img)
    
    # Try to use a system font, fallback to default
    try:
        font = ImageFont.truetype("arialbd.ttf", 44)  # Bold Arial for better readability
    except:
        try:
            font = ImageFont.truetype("arial.ttf", 44)
        except:
            try:
                font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 44)
            except:
                try:
                    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 44)
                except:
                    font = ImageFont.load_default()
    
    # Get text bounding box
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    
    # Center the text
    x = (width - text_width) // 2
    y = (height - text_height) // 2 - 5  # Adjust slightly upward for better vertical centering
    
    draw.text((x, y), text, font=font, fill=text_color)
    
    # Save the image
    os.makedirs('Resources', exist_ok=True)
    img.save(f'Resources/{filename}')
    print(f"Created: Resources/{filename}")

if __name__ == "__main__":
    # Create status text textures with colored text on transparent background
    # Size: 600x100 pixels (aspect ratio 6:1)
    create_text_image("Back-face culling: ON", "cull_on.png", text_color=(0, 255, 0, 255))
    create_text_image("Back-face culling: OFF", "cull_off.png", text_color=(255, 0, 0, 255))
    create_text_image("Depth test: ON", "depth_on.png", text_color=(0, 255, 0, 255))
    create_text_image("Depth test: OFF", "depth_off.png", text_color=(255, 0, 0, 255))
    
    print("\nAll textures created successfully!")
    print("Texture size: 600x100 pixels (aspect ratio 6:1)")
    print("Font size: 44px bold for better readability")
