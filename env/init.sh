set -x

virtualenv ./venv
source ./venv/bin/activate

pip install -r ./env/requirements_cli.txt

echo type: \"source ./venv/bin/activate\"
