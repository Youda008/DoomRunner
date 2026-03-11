import sys
import requests
import json

if len(sys.argv) < 3:
    print( "usage: {} <user> <repo>".format( sys.argv[0] ) )
    sys.exit(1)
    
user = sys.argv[1]
repo = sys.argv[2]

resp = requests.get( "https://api.github.com/repos/{}/{}/releases".format( user, repo ) )
js = json.loads( resp.text )

for release in js:
    for asset in release["assets"]:
        print(asset["name"] + ": " + str( asset["download_count"] ))
