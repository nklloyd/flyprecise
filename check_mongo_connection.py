import os
from dotenv import load_dotenv
from pymongo import MongoClient

def check_connection():
    load_dotenv()
    
    uri = os.getenv("MONGO_URI")
    db_name = os.getenv("DB_NAME")
    collection_name = os.getenv("COLLECTION_NAME")
    
    if not all([uri, db_name, collection_name]):
        print("Error: Missing one or more environment variables (MONGO_URI, DB_NAME, COLLECTION_NAME) in .env")
        return

    print(f"Connecting to: {uri}")
    print(f"Database: {db_name}")
    print(f"Collection: {collection_name}")
    
    try:
        client = MongoClient(uri)
        # The ping command is cheap and confirms connectivity
        client.admin.command('ping')
        print("Successfully connected to MongoDB!\n")
        
        db = client[db_name]
        collection = db[collection_name]
        
        count = collection.count_documents({})
        print(f"Found {count} documents in '{collection_name}'.")
        
        if count > 0:
            print("Reading documents...")
            for i, doc in enumerate(collection.find().limit(10)): # Limit to 10 for safety in this check script
                print(f"{i+1}: {doc}")
                
            if count > 10:
                print("\n... (showing only first 10 documents for brevity)")
        else:
            print("The collection is empty.")
            
    except Exception as e:
        print(f"Failed to connect or read from MongoDB: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    check_connection()
