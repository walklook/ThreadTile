import os, sys
sys.path.append('./libImage')
import Image
from biplist import *

def cutImageAndSave( imageFile, width, height, imageType ):
	img = Image.open( imageFile )
	( imgWidth, imgHeight ) = img.size
	row = imgHeight / height
	if imgHeight % height != 0:
		row += 1
	col = imgWidth / width
	if imgWidth % width != 0:
		col += 1

	plist = { 'Source' : { 'Column' : '' + str( col ), 'Row' : '' + str( row ), 'Size' : '{' + str( imgWidth ) + ',' + str( imgHeight ) + '}', 'TileSize' : '{' + str( width ) + ',' + str( height ) + '}' }, 'Tiles' : [] }

	for i in xrange( 0, row ):
		for j in xrange( 0, col ):
			if j == col - 1:
				boxZ = imgWidth
			else:
				boxZ = ( j + 1 ) * width
			if i == row - 1:
				boxW = imgHeight
			else:
				boxW = ( i + 1 ) * height

			box = ( j * width, i * height, boxZ, boxW )
			print box
			region = img.crop( box )
			index = imageFile.rfind( '.' )
			name = imageFile[ 0 : index ] + '_' + str( i ) + '_' + str( j ) + '.' + imageType
			imageName = name.rsplit( os.path.sep ).pop()
			print imageFile
			print name
			print imageName
			region.save( name )
			plist['Tiles'].append( { 'Name' : imageName, 'Rect' : '{{' + str( j * width ) + ', ' + str( i * height ) + '}, {' + str( boxZ - j * width ) + ', ' + str( boxW - i * height ) + '}}' } )

	try:
		plistName = imageFile.rsplit( os.path.sep ).pop()
		plistName = plistName[ 0 : plistName.rfind( '.' ) ] + '.plist'
		writePlist( plist, plistName )
	except ( InvalidPlistException, NotBinaryPlistException ), e:
		print "error:", e
                        
def main():
	if len( sys.argv ) == 5:
		cutImageAndSave( sys.argv[1], int( sys.argv[2] ), int( sys.argv[3] ), sys.argv[4] )
	else:
		print( 'The command should be "python TileCutter imageName width height imageType"' )

main()
  