# AniStudio Default Python Script
# This script demonstrates various Python features available in AniStudio
# Place this file in: /build/scripts/default_script.py

import math
import random
import sys
import os
from datetime import datetime

def main():
    """Main function demonstrating AniStudio Python integration"""
    
    print("=" * 50)
    print("Welcome to AniStudio Python Environment!")
    print("=" * 50)
    
    # System Information
    print(f"\nPython Version: {sys.version}")
    print(f"Executable: {sys.executable}")
    print(f"Platform: {sys.platform}")
    print(f"Current Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Math Demonstrations
    print(f"\nMathematical Constants:")
    print(f"   Pi: {math.pi:.6f}")
    print(f"   e (Euler): {math.e:.6f}")
    print(f"   Tau: {math.tau:.6f}")
    
    # Random Number Generation
    print(f"\nRandom Number Generation:")
    random_numbers = [random.randint(1, 100) for _ in range(8)]
    print(f"   Random integers (1-100): {random_numbers}")
    print(f"   Average: {sum(random_numbers) / len(random_numbers):.2f}")
    print(f"   Maximum: {max(random_numbers)}")
    print(f"   Minimum: {min(random_numbers)}")
    
    # String Manipulations
    print(f"\nString Manipulations:")
    message = "AniStudio: Media Creation Made Easy!"
    print(f"   Original: {message}")
    print(f"   Reversed: {message[::-1]}")
    print(f"   Uppercase: {message.upper()}")
    print(f"   Word Count: {len(message.split())} words")
    print(f"   Character Count: {len(message)} characters")
    
    # List Comprehensions
    print(f"\nList Comprehensions & Calculations:")
    squares = [x**2 for x in range(1, 11)]
    print(f"   Squares of 1-10: {squares}")
    
    cubes = [x**3 for x in range(1, 6)]
    print(f"   Cubes of 1-5: {cubes}")
    
    # Fibonacci sequence
    print(f"\nFibonacci Sequence (first 15 numbers):")
    fib_sequence = fibonacci_generator(15)
    print(f"   {fib_sequence}")
    
    # File system info
    print(f"\nFile System Information:")
    print(f"   Current Directory: {os.getcwd()}")
    print(f"   Python Path Count: {len(sys.path)} directories")
    
    # Media Creation Workflow Example
    print(f"\nMedia Creation Workflow Example:")
    project_settings = {
        "resolution": "1920x1080",
        "framerate": 30,
        "duration": "00:02:30",
        "format": "MP4",
        "quality": "High"
    }
    
    print("   Project Settings:")
    for key, value in project_settings.items():
        print(f"     {key.capitalize()}: {value}")
    
    # Advanced Python Features
    print(f"\nAdvanced Python Features:")
    
    # Lambda functions
    multiply = lambda x, y: x * y
    print(f"   Lambda (5 * 7): {multiply(5, 7)}")
    
    # List filtering
    even_numbers = list(filter(lambda x: x % 2 == 0, range(1, 21)))
    print(f"   Even numbers (1-20): {even_numbers}")
    
    # Dictionary comprehension
    number_words = {i: number_to_word(i) for i in range(1, 6)}
    print(f"   Number words: {number_words}")
    
    # Generator example
    print(f"\nGenerator Example (Powers of 2):")
    powers_of_two = [next(power_of_two_generator()) for _ in range(10)]
    print(f"   First 10 powers of 2: {powers_of_two}")
    
    print(f"\nScript execution completed successfully!")
    print("=" * 50)

def fibonacci_generator(n):
    """Generate fibonacci sequence up to n numbers"""
    sequence = []
    a, b = 0, 1
    for _ in range(n):
        sequence.append(a)
        a, b = b, a + b
    return sequence

def number_to_word(num):
    """Convert single digit number to word"""
    words = ["zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"]
    return words[num] if 0 <= num <= 9 else "unknown"

def power_of_two_generator():
    """Generator that yields powers of 2"""
    power = 0
    while True:
        yield 2 ** power
        power += 1

def create_sample_data():
    """Create sample data for media projects"""
    return {
        "scenes": [f"Scene_{i:03d}" for i in range(1, 11)],
        "characters": ["Hero", "Villain", "Sidekick", "Mentor"],
        "locations": ["Forest", "Castle", "Village", "Mountain"],
        "effects": ["Explosion", "Magic Sparkle", "Fade", "Zoom"]
    }

def demonstrate_file_operations():
    """Demonstrate file operations safely"""
    try:
        # This would work in a real environment
        sample_data = create_sample_data()
        print(f"   Sample project data: {len(sample_data)} categories")
        for category, items in sample_data.items():
            print(f"     {category.capitalize()}: {len(items)} items")
    except Exception as e:
        print(f"   File operation demo: {e}")

# Class example for object-oriented programming
class MediaProject:
    """Example class for media project management"""
    
    def __init__(self, name, duration=30, fps=30):
        self.name = name
        self.duration = duration
        self.fps = fps
        self.created_at = datetime.now()
        self.scenes = []
    
    def add_scene(self, scene_name, duration):
        """Add a scene to the project"""
        self.scenes.append({"name": scene_name, "duration": duration})
    
    def get_total_scenes(self):
        """Get total number of scenes"""
        return len(self.scenes)
    
    def get_total_duration(self):
        """Get total duration of all scenes"""
        return sum(scene["duration"] for scene in self.scenes)
    
    def __str__(self):
        return f"MediaProject('{self.name}', {self.get_total_scenes()} scenes, {self.get_total_duration()}s)"

def demonstrate_oop():
    """Demonstrate object-oriented programming"""
    print(f"\nObject-Oriented Programming Example:")
    
    # Create a media project
    project = MediaProject("My Animation", fps=60)
    project.add_scene("Opening", 5)
    project.add_scene("Main Action", 15)
    project.add_scene("Closing", 3)
    
    print(f"   {project}")
    print(f"   Created: {project.created_at.strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"   FPS: {project.fps}")

if __name__ == "__main__":
    main()
    demonstrate_oop()
    demonstrate_file_operations()
    
    print(f"\nReady for your AniStudio projects!")