import os
import subprocess

BASE_DIR = './data'

def choose_from_list(prompt, options):
    print(f"\n{prompt}")
    for i, opt in enumerate(options, 1):
        print(f"{i}. {opt}")
    while True:
        choice = input("Enter choice number: ")
        if choice.isdigit():
            idx = int(choice) - 1
            if 0 <= idx < len(options):
                return options[idx]
        print("Invalid choice, try again.")

def main():
    quantities = sorted(os.listdir(BASE_DIR))
    quantity = choose_from_list("Choose quantity:", quantities)

    dup_dir = os.path.join(BASE_DIR, quantity)
    duplicities = sorted(os.listdir(dup_dir))
    duplicity = choose_from_list("Choose duplicity:", duplicities)

    order_dir = os.path.join(dup_dir, duplicity)
    orders = sorted(os.listdir(order_dir))
    order = choose_from_list("Choose order:", orders)

    files_dir = os.path.join(order_dir, order)
    files = sorted(f for f in os.listdir(files_dir) if f.endswith('.txt'))
    file_choice = choose_from_list("Choose file ID:", files)

    full_path = os.path.join(files_dir, file_choice)
    print(f"\nSelected file: {full_path}")

    # === NEW: Run C executable and pass file path ===
    print("\nRunning sorting benchmark on selected file...\n")

    executable = "main.exe"
    try:
        subprocess.run([executable, full_path])
    except FileNotFoundError:
        print(f"Error: Cannot find executable '{executable}'.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
