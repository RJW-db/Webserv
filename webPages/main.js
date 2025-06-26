function goToPhotoViewer() {
  window.location.href = 'DELETE_photo.html';
}

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
          const img = document.createElement('img');
          img.src = '/' + filename.trim();
          img.alt = filename.trim();
          gallery.appendChild(img);
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