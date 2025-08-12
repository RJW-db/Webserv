window.addEventListener('DOMContentLoaded', function() {
  // Handle file upload
  document.getElementById('uploadForm').addEventListener('submit', function(e) {
    e.preventDefault();

    const files = document.getElementById('fileInput').files;
    const gallery = document.getElementById('gallery');
    const status = document.getElementById('status');
    status.textContent = '';

    // Upload each file individually
    Array.from(files).forEach(file => {
      const formData = new FormData();
      formData.append('myfile', file);

      fetch('/upload', {
        method: 'POST',
        body: formData
      })
      .then(response => response.text())
      .then(filename => {
        if (filename && filename !== 'Upload failed.') {
          status.textContent = 'Upload successful!';

          // Create a wrapper div for filename, image, and delete button
          const wrapper = document.createElement('div');
          wrapper.style.position = 'relative';
          wrapper.style.display = 'inline-block';
          wrapper.style.margin = '10px';

          // Filename label above the image
          const label = document.createElement('div');
          label.textContent = filename.trim().split('/').pop();
          label.style.textAlign = 'center';
          label.style.marginBottom = '5px';
          label.style.fontSize = '16px';

          const img = document.createElement('img');
          img.src = '/' + filename.trim();
          img.alt = filename.trim();

          const delBtn = document.createElement('button');
          delBtn.textContent = 'Delete';
          delBtn.style.position = 'absolute';
          delBtn.style.top = '10px';
          delBtn.style.right = '10px';
          delBtn.onclick = function() {
            // Only use the filename, not the path
            const cleanFilename = filename.trim().split('/').pop();
            fetch('/upload/' + encodeURIComponent(cleanFilename), {
              method: 'DELETE'
            })
            .then(res => {
              if (res.ok) {
                wrapper.remove();
              } else {
                alert('Failed to delete image.');
              }
            })
            .catch(() => alert('Error deleting image.'));
          };

          wrapper.appendChild(label);   // Add label above image
          wrapper.appendChild(img);
          wrapper.appendChild(delBtn);
          gallery.appendChild(wrapper);
        } else {
          status.textContent = 'Upload failed.';
        }
      })
      .catch(error => {
        status.textContent = 'Error: ' + error;
      });
    });

    // Optionally clear the file input
    document.getElementById('fileInput').value = '';
  });

  // Handle CGI file upload
  document.getElementById('cgiUploadForm').addEventListener('submit', function(e) {
    e.preventDefault();

    const files = document.getElementById('cgiFileInput').files;
    const gallery = document.getElementById('gallery');
    const status = document.getElementById('cgiStatus');
    status.textContent = '';

    Array.from(files).forEach(file => {
      const formData = new FormData();
      formData.append('myfile', file);

      fetch('/cgi-bin/upload.py?upload_dir=upload&public_url_base=/upload', {
      // fetch('/cgi-bin/cgi?upload_dir=upload&public_url_base=/upload', {
        method: 'POST',
        body: formData
      })
      .then(response => response.text())
      .then(result => {
        if (result && result !== 'Upload failed.') {
          status.textContent = 'CGI Upload successful!';
          status.style.color = 'green';

          const filename = result.trim();
          const cleanFilename = filename.split('/').pop();

          const wrapper = document.createElement('div');
          wrapper.style.position = 'relative';
          wrapper.style.display = 'inline-block';
          wrapper.style.margin = '10px';

          const label = document.createElement('div');
          label.textContent = cleanFilename;
          label.style.textAlign = 'center';
          label.style.marginBottom = '5px';
          label.style.fontSize = '16px';

          const img = document.createElement('img');
          img.src = '/' + filename.trim();
          img.alt = filename.trim();

          const delBtn = document.createElement('button');
          delBtn.textContent = 'Delete';
          delBtn.style.position = 'absolute';
          delBtn.style.top = '10px';
          delBtn.style.right = '10px';
          delBtn.onclick = function() {
            fetch('/upload/' + encodeURIComponent(cleanFilename), {
              method: 'DELETE'
            })
            .then(res => {
              if (res.ok) {
                wrapper.remove();
              } else {
                alert('Failed to delete image.');
              }
            })
            .catch(() => alert('Error deleting image.'));
          };

          wrapper.appendChild(label);
          wrapper.appendChild(img);
          wrapper.appendChild(delBtn);
          gallery.appendChild(wrapper);

        } else {
          status.textContent = 'CGI Upload failed.';
          status.style.color = '#b00';
        }
      })
      .catch(error => {
        status.textContent = 'CGI Error: ' + error;
        status.style.color = '#b00';
      });
    });

    // Clear the file input
    document.getElementById('cgiFileInput').value = '';
  });


  // Handle delete by filename
  document.getElementById('deletePathBtn').addEventListener('click', function() {
    const filename = document.getElementById('deleteFileInput').value.trim();
    const statusDiv = document.getElementById('deletePathStatus');
    statusDiv.textContent = '';
    if (!filename) {
      statusDiv.textContent = 'Please enter a filename.';
      return;
    }
    fetch('/upload/' + encodeURIComponent(filename), {
      method: 'DELETE'
    })
    .then(res => {
      if (res.ok) {
        statusDiv.style.color = 'green';
        statusDiv.textContent = 'Delete successful!';

        // Remove the image from the gallery if present
        const gallery = document.getElementById('gallery');
        const wrappers = gallery.children;
        const filenameToDelete = filename.split('/').pop();
        for (let i = wrappers.length - 1; i >= 0; i--) {
          const label = wrappers[i].querySelector('div');
          if (label && label.textContent === filenameToDelete) {
            gallery.removeChild(wrappers[i]);
            break;
          }
        }
      } else {
        statusDiv.style.color = '#b00';
        statusDiv.textContent = 'Delete failed.';
      }
    })
    .catch(() => {
      statusDiv.style.color = '#b00';
      statusDiv.textContent = 'Error sending delete request.';
    });
  });
});