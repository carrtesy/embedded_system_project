from flask import Flask, render_template, request
import threading, fcntl, array, time 

app =Flask(__name__)

@app.route('/')
def main_page():
    return render_template("main.html")

@app.route('/led_set',methods=['POST'])
def led_set():
    led_string=""
    for i in range(4):
        led_string += request.form['led_'+str(i)]
        
    thread = threading.Thread(target=led_function,args=(led_string,))
    thread.start()
    return render_template("led_set.html", led_string = led_string)

def led_function(led_string):
    with open("/dev/rpikey","w") as rpikey:
        for c in led_string:
            if c =='r': ar=array.array('l',[0,0,1])
            if c =='g': ar=array.array('l',[0,1,0])
            if c =='b': ar=array.array('l',[1,0,0])
            if c =='o': ar=array.array('l',[0,0,0])
            
            fcntl.ioctl(rpikey,101,ar,0)
            time.sleep(1)
            
if __name__ =="__main__":
    app.run(host="0.0.0.0",debug=True)
