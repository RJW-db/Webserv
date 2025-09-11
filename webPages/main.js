window.addEventListener('DOMContentLoaded', function() {
  // Display existing images in the gallery on page load
  fetch('/upload/list')
    .then(res => {
      if (!res.ok) throw new Error('Failed to fetch file list');
      return res.text();
    })
    .then(text => {
      const gallery = document.getElementById('gallery');
      const files = text.trim().split('\n');
      files.forEach(filename => {
        if (filename) {
          const wrapper = document.createElement('div');
          wrapper.style.position = 'relative';
          wrapper.style.display = 'inline-block';
          wrapper.style.margin = '10px';

          const label = document.createElement('div');
          label.textContent = filename;
          label.style.textAlign = 'center';
          label.style.marginBottom = '5px';
          label.style.fontSize = '16px';

          const img = document.createElement('img');
          img.src = `/upload/${encodeURIComponent(filename)}`;
          img.alt = filename;

          const delBtn = document.createElement('button');
          delBtn.textContent = 'Delete';
          delBtn.style.position = 'absolute';
          delBtn.style.top = '10px';
          delBtn.style.right = '10px';
          delBtn.onclick = function() {
            fetch('/upload/' + encodeURIComponent(filename), {
              method: 'DELETE'
            })
            .then(res => {
              if (res.ok) {
                wrapper.remove();
              } else {
                alert('Failed to delete image.');
              }
            })
            .catch (() => alert('Error deleting image.'));
          };

          // Append children to wrapper
          wrapper.appendChild(label);
          wrapper.appendChild(img);
          wrapper.appendChild(delBtn);

          // Append wrapper to gallery
          gallery.appendChild(wrapper);
        }
      });
    })
    .catch(err => {
      console.error('Error loading file list:', err);
    });

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
      .then(response => response.text().then(text => ({ok: response.ok, text})))
      .then(({ok, text}) => {
        if (ok && text && text !== 'Upload failed.' && !text.startsWith('<html')) {
          status.textContent = 'Upload successful!';

          // Create a wrapper div for filename, image, and delete button
          const wrapper = document.createElement('div');
          wrapper.style.position = 'relative';
          wrapper.style.display = 'inline-block';
          wrapper.style.margin = '10px';

          // Filename label above the image
          const label = document.createElement('div');
          label.textContent = text.trim().split('/').pop();
          label.style.textAlign = 'center';
          label.style.marginBottom = '5px';
          label.style.fontSize = '16px';

          const img = document.createElement('img');
          img.src = '/' + text.trim();
          img.alt = text.trim();

          const delBtn = document.createElement('button');
          delBtn.textContent = 'Delete';
          delBtn.style.position = 'absolute';
          delBtn.style.top = '10px';
          delBtn.style.right = '10px';
          delBtn.onclick = function() {
            const cleanFilename = text.trim().split('/').pop();
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
            .catch (() => alert('Error deleting image.'));
          };

          wrapper.appendChild(label);
          wrapper.appendChild(img);
          wrapper.appendChild(delBtn);
          gallery.appendChild(wrapper);
        } else {
          status.textContent = '';
          const errorBox = document.createElement('div');
          errorBox.textContent = 'Upload failed: Server error or invalid response.';
          errorBox.style.color = '#b00';
          errorBox.style.background = '#fff0f0';
          errorBox.style.border = '1px solid #b00';
          errorBox.style.padding = '10px';
          errorBox.style.margin = '10px';
          errorBox.style.fontWeight = 'bold';
          gallery.appendChild(errorBox);
        }
      })
      .catch (error => {
        status.textContent = '';
        const errorBox = document.createElement('div');
        errorBox.textContent = 'Upload failed: ' + error;
        errorBox.style.color = '#b00';
        errorBox.style.background = '#fff0f0';
        errorBox.style.border = '1px solid #b00';
        errorBox.style.padding = '10px';
        errorBox.style.margin = '10px';
        errorBox.style.fontWeight = 'bold';
        gallery.appendChild(errorBox);
      });
    });

    document.getElementById('fileInput').value = '';
  });

  // Handle CGI delete by filename
  document.getElementById('cgiDeletePathBtn').addEventListener('click', function() {
    const filename = document.getElementById('cgiDeleteFileInput').value.trim();
    const statusDiv = document.getElementById('cgiDeletePathStatus');
    statusDiv.textContent = '';
    if (!filename) {
      statusDiv.textContent = 'Please enter a filename.';
      return;
    }
    fetch('/cgi-bin/upload.py?filename=' + encodeURIComponent(filename), {
      method: 'DELETE'
    })
    .then(res => {
      if (res.ok) {
        statusDiv.style.color = 'green';
        statusDiv.textContent = 'CGI Delete successful!';

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
        statusDiv.textContent = 'CGI Delete failed.';
      }
    })
    .catch (() => {
      statusDiv.style.color = '#b00';
      statusDiv.textContent = 'Error sending CGI delete request.';
    });
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
        method: 'POST',
        body: formData
      })
      .then(response => response.text().then(text => ({ok: response.ok, text})))
      .then(({ok, text}) => {
        if (ok && text && text !== 'Upload failed.' && !text.startsWith('<html')) {
          status.textContent = 'CGI Upload successful!';
          status.style.color = 'green';

          const filename = text.trim();
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
            .catch (() => alert('Error deleting image.'));
          };

          wrapper.appendChild(label);
          wrapper.appendChild(img);
          wrapper.appendChild(delBtn);
          gallery.appendChild(wrapper);

        } else {
          status.textContent = '';
          const errorBox = document.createElement('div');
          errorBox.textContent = 'CGI Upload failed: Server error or invalid response.';
          errorBox.style.color = '#b00';
          errorBox.style.background = '#fff0f0';
          errorBox.style.border = '1px solid #b00';
          errorBox.style.padding = '10px';
          errorBox.style.margin = '10px';
          errorBox.style.fontWeight = 'bold';
          gallery.appendChild(errorBox);
        }
      })
      .catch (error => {
        status.textContent = '';
        const errorBox = document.createElement('div');
        errorBox.textContent = 'CGI Upload failed: ' + error;
        errorBox.style.color = '#b00';
        errorBox.style.background = '#fff0f0';
        errorBox.style.border = '1px solid #b00';
        errorBox.style.padding = '10px';
        errorBox.style.margin = '10px';
        errorBox.style.fontWeight = 'bold';
        gallery.appendChild(errorBox);
      });
    });

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
    .catch (() => {
      statusDiv.style.color = '#b00';
      statusDiv.textContent = 'Error sending delete request.';
    });
  });

  const toggleBtn = document.getElementById('toggleThemeBtn');
  fetch('/get-theme', { credentials: 'same-origin' })
    .then(res => res.json())
    .then(data => {
      let isLight = data.theme === 'light';
      document.body.classList.toggle('light-mode', isLight);
      toggleBtn.textContent = isLight ? 'Switch to Dark Mode' : 'Switch to Light Mode';
      toggleBtn.dataset.isLight = isLight ? '1' : '0';
    });

  toggleBtn.addEventListener('click', function() {
    let isLight = toggleBtn.dataset.isLight !== '1';
    document.body.classList.toggle('light-mode', isLight);
    toggleBtn.textContent = isLight ? 'Switch to Dark Mode' : 'Switch to Light Mode';
    toggleBtn.dataset.isLight = isLight ? '1' : '0';

    // Send theme update as POST form data
    const formData = new URLSearchParams();
    formData.append('theme', isLight ? 'light' : 'dark');
    fetch('/set-theme', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: formData.toString(),
      credentials: 'same-origin'
    });
  });
});