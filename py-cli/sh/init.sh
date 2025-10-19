set -x

virtualenv ./venv
source ./venv/bin/activate

pip install -r ./requirements_dev.txt

echo type: \"source ./venv/bin/activate\"
