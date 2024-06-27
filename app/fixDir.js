const fs = require('fs')
const path = require('path')

const sourceFilePath = './out/preload/preload.js'
const destinationFilePath = './out/preload.js'
const preloadFolderPath = './out/preload'

// Check if source file exists
if (fs.existsSync(sourceFilePath)) {
  // Create the directory if it doesn't exist
  const destinationDir = path.dirname(destinationFilePath)
  if (!fs.existsSync(destinationDir)) {
    fs.mkdirSync(destinationDir, { recursive: true })
  }

  // Move the file
  fs.rename(sourceFilePath, destinationFilePath, (err) => {
    if (err) {
      console.error(`Error moving file: ${err}`)
    } else {
      console.log('File moved successfully.')

      // Check if preload folder is empty
      fs.readdir(preloadFolderPath, (err, files) => {
        if (err) {
          console.error(`Error reading preload folder: ${err}`)
          return
        }
        // If folder is empty, delete it
        if (files.length === 0) {
          fs.rmdir(preloadFolderPath, (err) => {
            if (err) {
              console.error(`Error deleting preload folder: ${err}`)
            } else {
              console.log('Empty preload folder deleted.')
            }
          })
        }
      })
    }
  })
} else {
  console.error('Source file does not exist.')
}
