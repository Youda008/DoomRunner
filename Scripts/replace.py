import sys


def main():
    if len(sys.argv) < 3:
        print("Usage: <command> | python3 replace.py <old_word> <new_word>")
        return

    old_word = sys.argv[1]
    new_word = sys.argv[2]

    # sys.stdin reads the piped input line by line
    for line in sys.stdin:
        # line.replace() handles the swap
        # end='' prevents adding extra double-newlines
        print(line.replace(old_word, new_word), end='')


if __name__ == "__main__":
    main()
