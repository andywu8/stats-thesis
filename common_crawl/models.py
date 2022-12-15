import sqlite3 as sql

def insert_dummy_data():
    print("insert dummy data")
    con = sql.connect("database.db")
    cur = con.cursor()
    query = "INSERT INTO crawl_data (author, comment) VALUES (?, ?)"
    cur.execute(query, ["test author", "test comment"])
    con.commit()
    con.close()
def insert_data(author, comment):
    con = sql.connect("database.db")
    cur = con.cursor()
    query = "INSERT INTO crawl_data (author, comment) VALUES (?, ?)"
    cur.execute(query, [author, comment])
    con.commit()
    con.close()

def retrieve_data():
    con = sql.connect("database.db")
    cur = con.cursor()
    query = "SELECT * FROM crawl_data"
    cur.execute(query)
    rows = cur.fetchall()
    
    print("Check that retrieve data works")
    for row in rows:
        print("row", row)
    con.close()
    return rows

def select_most_frequent():
    con = sql.connect("database.db")
    cur = con.cursor()
    query = "SELECT author, COUNT(author) FROM crawl_data "
    query += "GROUP BY author "
    query += "ORDER BY COUNT(author) DESC LIMIT 10"
    cur.execute(query)
    rows = cur.fetchall()
    for row in rows:
        print("row", row)
    return rows

if __name__ == "__main__":
    retrieve_data()