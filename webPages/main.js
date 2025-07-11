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