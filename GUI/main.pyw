import sys
import BaseHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
import webbrowser
import threading
from Tkinter import *
from tkMessageBox import askretrycancel
import json
import os
from datetime import datetime
import serial
from serial_ports import serial_ports
import csv
import tkFileDialog

#A sublcass of SimpleHTTPRequestHandler that does not print to console
class SilentHandler(SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        return

lock = threading.RLock()
empty = None
data = []
csvdata = []
counter = 0

def verify_mbed():
	global empty

	ports = serial_ports()
	if len(ports) == 0 :
		if empty == None:
			empty = Tk()
			empty.withdraw()
		if askretrycancel('DigiCath Warning!', 'Please connect the mbed.'):
			verify_mbed()
		else:
			sys.exit(0)

verify_mbed()
if empty != None:
	empty.quit()
	empty.destroy()

ser = serial.Serial(serial_ports()[0], timeout = 0.5)
serialThreadStatus = True
flow = 0

def read_from_port(ser):
	global serialThreadStatus, flow, counter

	#sets default value of prev_signal
	prev_signal = '';
	while serialThreadStatus:
		with lock:
			ser.flushInput()
			signal = ser.readline()[0:1]					
			#set default prev_signal value correctly
			if (signal=='A' or signal=='B') and len(prev_signal)==0:
				prev_signal = signal
			#processes signal when value switches
			if (signal=='B' and prev_signal=='A') or (signal=='A' and prev_signal=='B'):
				counter += 5
				prev_signal = signal
			ser.write(str(flow).zfill(3) + '\n')
thread = threading.Thread(target = read_from_port, args = (ser,))
thread.start()

HandlerClass = SilentHandler
ServerClass  = BaseHTTPServer.HTTPServer
Protocol     = 'HTTP/1.0'
server_address = ('127.0.0.1', 8000)
HandlerClass.protocol_version = Protocol
httpd = ServerClass(server_address, HandlerClass)
sa = httpd.socket.getsockname()

class App(threading.Thread):
	def __init__(self):
			threading.Thread.__init__(self)
			self.start()
	def callback(self):
		self.root.quit()
		httpd.shutdown()
		with lock:
			global serialThreadStatus
			serialThreadStatus = False
	def savecsv(self):
		f = tkFileDialog.asksaveasfilename(defaultextension='.csv', initialfile = 'digitcathdata')
		if len(f) > 0:
			with open(f, 'wb') as csvfile:
				writer = csv.writer(csvfile, delimiter=' ',quotechar='"', quoting=csv.QUOTE_MINIMAL)
				writer.writerows(csvdata)
	def flow(self, val):
		with lock:
			global flow
			flow = val
	def run(self):
			self.root = Tk()
			self.root.title('Digital Catheter')
			self.root.iconbitmap(default = 'transparent.ico')
			self.root.protocol('WM_DELETE_WINDOW', self.callback)

			self.quit = Button(text = 'CLOSE', command = self.callback)
			self.open = Button(text = 'OPEN CHART', command = self.open)
			self.csv = Button(text = 'EXPORT TO .CSV', command = self.savecsv)
			self.flow = Scale(from_ = 0, to = 100, orient = HORIZONTAL, command = self.flow)
			self.label = Label(text = 'FLOW RATE')

			self.label.grid(row = 0, column = 0)
			self.flow.grid(row = 0, column = 1, columnspan = 2, sticky = W+E)
			self.open.grid(row = 1, column = 0, padx = 2, pady = 2)
			self.csv.grid(row = 1, column = 1, padx = 2, pady = 2)
			self.quit.grid(row = 1 , column = 2, padx = 2, pady = 2)

			self.root.resizable(width=FALSE, height=FALSE)
			self.update()

			self.root.mainloop()
	def open(self):
		webbrowser.open_new_tab('http://localhost:8000')
	def update(self):
		global counter
		time = datetime.now()
		millisec = (time - datetime(1970,1,1,0,0,0,0)).total_seconds() * 1000
		data.append((millisec, counter))
		csvdata.append((time.strftime('%Y-%m-%d %H:%M:%S.%f'), counter))
		counter = 0
		with open('data.json', 'w+') as outfile:
			json.dump({'results': data}, outfile)
		self.root.after(10000,self.update)

if os.path.isfile('data.json'):
	os.remove('data.json')
app = App()
httpd.serve_forever()