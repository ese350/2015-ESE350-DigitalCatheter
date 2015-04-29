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
	prev_signal = "";
	while serialThreadStatus:
		with lock:
			ser.flushInput()
			signal = ser.readline()
			if (len(signal) > 0) && (signal != prev_signal):
				print "mbed: " + signal

				#only switching the signal corresponds to a shock
				#the first signal thus does not need to be registered as a shock
				if prev_signal != "":
					counter += 5
				prev_signal = signal
			ser.write(str(flow) + '\n')
thread = threading.Thread(target = read_from_port, args = (ser,))
thread.start()


HandlerClass = SimpleHTTPRequestHandler
ServerClass  = BaseHTTPServer.HTTPServer
Protocol     = "HTTP/1.0"
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
		f = tkFileDialog.asksaveasfilename(defaultextension=".csv", initialfile = "digitcathdata")
		if len(f) > 0:
			with open(f, 'wb') as csvfile:
				writer = csv.writer(csvfile, delimiter=' ',quotechar='"', quoting=csv.QUOTE_MINIMAL)
				writer.writerows(csvdata)
	def flow0(self):
		print 'flow0'
		with lock:
			global flow
			flow = 0
	def flow1(self):
		print 'flow1'
		with lock:
			global flow
			flow = 1
	def flow2(self):
		print 'flow2'
		with lock:
			global flow
			flow = 2
	def flow3(self):
		print 'flow3'
		with lock:
			global flow
			flow = 3
	def run(self):
			self.root = Tk()
			self.root.title('Digital Catheter')
			self.root.protocol('WM_DELETE_WINDOW', self.callback)

			self.button = Button(text = 'QUIT', command = self.callback)
			self.button.pack(side = LEFT)
			self.open = Button(text = 'OPEN CHARTS', command = self.open)
			self.open.pack(side = LEFT)
			self.csv = Button(text = 'SAVE DATA TO CSV', command = self.savecsv)
			self.csv.pack(side = LEFT)
			self.update()

			self.flow0 = Button(text = 'NO FLOW', command = self.flow0)
			self.flow1 = Button(text = 'FLOW 1', command = self.flow1)
			self.flow2 = Button(text = 'FLOW 2', command = self.flow2)
			self.flow3 = Button(text = 'FLOW 3', command = self.flow3)
			self.flow0.pack(side = LEFT)
			self.flow1.pack(side = LEFT)
			self.flow2.pack(side = LEFT)
			self.flow3.pack(side = LEFT)

			self.root.mainloop()
	def open(self):
		webbrowser.open_new_tab('http://localhost:8000')
	def update(self):
		global counter
		print 'counter: '+ str(counter)
		time = datetime.now()
		millisec = (time - datetime(1970,1,1,0,0,0,0)).total_seconds() * 1000
		data.append((millisec, counter))
		csvdata.append((time.strftime('%Y-%m-%d %H:%M:%S.%f'), counter))
		counter = 0
		with open('data.json', 'w+') as outfile:
			json.dump({"results": data}, outfile)
		self.root.after(10000,self.update)

if os.path.isfile('data.json'):
	os.remove('data.json')
app = App()
httpd.serve_forever()