#! /bin/sh

# This is the demo script for NP project 3. You should use this script to automatically compile and make the executables
# as the spec says.
# If nothing goes wrong, some urls will shown and you should manually use a browser to visit them.

# Print a string with color modified.
# e.g., $ ERROR "This is an error".
ERROR() {
  echo "\e[91m[ERROR] $1\e[39m"
}

# Abbreviate from SUCCESS
SUCCESS() {
  echo "\e[92m[SUCCESS] $1\e[39m"
}

INFO() {
  echo "\e[93m[INFO] $1\e[39m"
}

# Define variables
NP_SCRIPT_PATH=$(readlink -f "$0")
NP_SCRIPT_DIR=$(dirname "$NP_SCRIPT_PATH")
DEMO_DIR="$1"

# Fetching student id, by args or by searching
if [ -n "$2" ]; then
  STUDENT_ID=$2
else
  STUDENT_ID=$(ldapsearch -LLLx "uid=$USER" csid | tail -n 2 | head -n 1 | cut -d " " -f 2)
fi

# Copy student's source code and utilities to the demo directory.
cp -r "$NP_SCRIPT_DIR/src/$STUDENT_ID" "$DEMO_DIR/src"
mkdir "$DEMO_DIR/working_dir" && cp -r "$NP_SCRIPT_DIR/working_dir/"* "$DEMO_DIR/working_dir"
cp -r "$NP_SCRIPT_DIR/port.py" "$DEMO_DIR"

# Copy files to default-http_server directory.
mkdir -p "$HOME/public_html/npdemo5" && cp -r "$NP_SCRIPT_DIR/working_dir/"* "$HOME/public_html/npdemo5"
chmod +x "$HOME/public_html/npdemo5/"*.cgi
chmod +x "$DEMO_DIR/working_dir/"*.cgi


# Change directory into the demo directory.
# Compile, and let the student's programs do it's work.
if cd "$DEMO_DIR"; then
  INFO "Compiling..."
  if (make -C "$DEMO_DIR/src"); then
    SUCCESS "Compilation completed!"
  else
    ERROR "Your project cannot be compiled by make"
    exit 1
  fi
  # Link socks.conf to working directory
  if [ -f "$NP_SCRIPT_DIR/socks.conf" ]; then
    # ln -sf "$NP_SCRIPT_DIR/socks.conf" "$DEMO_DIR/working_dir/socks.conf"
    cp "$NP_SCRIPT_DIR/socks.conf" "$DEMO_DIR/working_dir/socks.conf"
    SUCCESS "Creat socks.conf!"
  else
    ERROR "socks.conf not found in source directory"
    exit 1
  fi


  if ! (cp src/pj5.cgi src/socks_server "$DEMO_DIR/working_dir"); then
    ERROR "Failed to copy the generated executables to the demo directory (Did you name them right?)."
    exit 1
  fi
  if ! (cp src/pj5.cgi "$HOME/public_html/npdemo5"); then
    ERROR "Cannot copy your pj5.cgi to ~/public_html/npdemo5 (Did you name it right?)."
    exit 1
  fi

  SOCKS_SERVER_PORT=$(python3 port.py)
  SUCCESS "Your socks_server is listening at port: $SOCKS_SERVER_PORT"
  echo ""
  INFO "Test pj5.cgi"
  echo "      http://$(hostname).cs.nycu.edu.tw/~$(whoami)/npdemo5/panel_socks.cgi"
  echo ""
  INFO "Connect to NTHU by Firefox with socks_server. [default is permit]"
  echo "      https://www.nthu.edu.tw/"
  INFO "Connect to NYCU by Firefox with socks_server. [default is not permit]"
  echo "      https://www.nycu.edu.tw/"
  echo ""
  ./start-container.sh -c "cd working_dir || exit 1; ./socks_server $SOCKS_SERVER_PORT"

else
  ERROR "Cannot change to the demo directory!"
  exit
fi
