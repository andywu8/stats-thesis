"""
Run this file to create SQL tables
"""
import sqlite3
from sqlite3 import OperationalError

conn = sqlite3.connect('database.db')
c = conn.cursor()

fd = open('schema.sql', 'r')
sqlFile = fd.read()
fd.close()

sqlCommands = sqlFile.split(';')

for command in sqlCommands:
    try:
        c.execute(command)
    except OperationalError as msg:
        print("Command skipped: ", msg)