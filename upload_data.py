"""
Get the last id/day/data and perform a push of a new row of data
"""
from __future__ import print_function
from googleapiclient.discovery import build
from httplib2 import Http
from oauth2client import file, client, tools
import time,datetime
import sys

# Setup the Sheets API
SCOPES = 'https://www.googleapis.com/auth/spreadsheets'

store = file.Storage('/home/pi/BID_Recyling/token.json')
creds = store.get()
if not creds or creds.invalid:
    flow = client.flow_from_clientsecrets('/home/pi/BID_Recyling/credentials.json', SCOPES)
    creds = tools.run_flow(flow, store)
service = build('sheets', 'v4', http=creds.authorize(Http()))



#INTENTA OBTENER LOS VALORES EXISTENTENTES AL SHEET
SPREADSHEET_ID = '10ZP4AgFVVS6fVF1O8l0wMUE5o04CnzUM9C_Ncdo88Eo'
RANGE_NAME = 'cocina!A2:C'

try:
    
    result = service.spreadsheets().values().get(spreadsheetId=SPREADSHEET_ID,
                                             range=RANGE_NAME).execute()
    values = result.get('values', [])
except:
    print('Error leyendo datos existentes')

if not values:
    print('Error no se encontaron datos')
else:	
	last_position = len(values) + 1    
	last_id = values[last_position-2][0]
	last_day = values[last_position-2][1]
	last_data = values[last_position-2][2]
	#print('Last id: %s' % (last_id))
	#print('Last day: %s' % (last_day))
	#print('Last data: %s' % (last_data))
	print('Ultima posicion tabla: %d' % (last_position))        

    
        
        
     
#OBTIENE LOS VALORES PARA AGREGAR AL SHEET
timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%d/%m/%Y %H:%M:%S')	
values = [
    [
        1,
        timestamp, 
        sys.argv[1]
    ]
    ,
    [
        2,
        timestamp, 
        sys.argv[2]
    ],
    [
        3,
        timestamp, 
        sys.argv[3]
    ],
    [
        4,
        timestamp, 
        sys.argv[4]
    ],
    #[
    #    5,
    #    timestamp, 
    #    sys.argv[5]
    ##]
    # Additional rows ...
]
body = {
    'values': values
}

range_name = 'cocina!A'
range_name = range_name + str(last_position+1)
value_input_option = 'USER_ENTERED'
spreadsheet_id = '10ZP4AgFVVS6fVF1O8l0wMUE5o04CnzUM9C_Ncdo88Eo'

#Intenta enviar los datos a la nube
try:
    result = service.spreadsheets().values().append(
        spreadsheetId=spreadsheet_id, range=range_name,
        valueInputOption=value_input_option, body=body).execute()
    print('{0} Celdas subidas correctamente!.'.format(result \
                                       .get('updates') \
                                       .get('updatedCells')));
except:
    print('Error al subir los datos a la nube!')
