window.addEventListener('DOMContentLoaded', function() {
  document.getElementById('uploadForm').addEventListener('submit', function(e) {
    e.preventDefault();

    const files = document.getElementById('fileInput').files;
    const gallery = document.getElementById('gallery');
    let status = document.getElementById('status');
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

          // Create a wrapper div for image and delete button
          const wrapper = document.createElement('div');
          wrapper.style.position = 'relative';
          wrapper.style.display = 'inline-block';

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
});