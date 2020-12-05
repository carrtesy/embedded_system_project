from flask import Flask
from flask import render_template
from datetime import datetime as dt

app = Flask(__name__)

@app.route('/')
def hello():
    return "Hello World"

@app.route('/hello/<user>')
def hello_name(user):
    return render_template('hello.html'
            ,name=user
            ,time=dt.now().time())

@app.route('/time')
def time_page():
    time_str = str(dt.now().time())
    return "Time: " + time_str

@app.route('/hello/<user>')
def hello_page(user):
    return "Hello " + user

@app.route('/double_this/<int:value>')
def double_page(value):
    return "Result: {}".format(value*2)

if __name__ == '__main__':
    app.run(host="0.0.0.0")
