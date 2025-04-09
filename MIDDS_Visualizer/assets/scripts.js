let requestCloseOfErrorDiv = false;
let closeErrorDivTimeout;

window.dash_clientside = Object.assign({}, window.dash_clientside, {
    clientside: {
        closeErrorOnClick: function(errorDivClassName) {
            const errorDiv = document.getElementById('error-message-div');
            if (!errorDiv) return;

            // If the div already is hidden, nothing to do here.
            if(errorDiv.classList.contains('hidden')) return;

            // Clear the previous timeout first.
            clearTimeout(closeErrorDivTimeout);

            // Automatically closes the error div after a certain time.
            closeErrorDivTimeout = setTimeout(function() {
                if(!errorDiv.classList.contains('hidden')) {
                    requestCloseOfErrorDiv = true;
                }
            }, 5000);

            errorDiv.addEventListener('mouseover', function(evt) {
                // If the mouse is over the error div, do not close it.
                clearTimeout(closeErrorDivTimeout);
            });

            errorDiv.addEventListener('mouseout', function(evt) {
                // If the mouse has exited the div, give some time to close it.
                closeErrorDivTimeout = setTimeout(function() {
                    if(!errorDiv.classList.contains('hidden')) {
                        requestCloseOfErrorDiv = true;
                    }
                }, 5000);
            });

            const closeErrorButton = errorDiv.querySelector('.close-error-div');
            if(!closeErrorButton) return;

            if(!closeErrorButton.dataset.listenerAdded) {
                closeErrorButton.addEventListener('click', function(evt) {
                    if(!errorDiv.classList.contains('hidden')) {
                        requestCloseOfErrorDiv = true;
                    }
                    // Prevent multiple bindings.
                    closeErrorButton.dataset.listenerAdded = 'true';
                }, false);
            }
        },

        updateInterfaceFromClient: function(n_interval, clientServerStore) {
            dictStore = JSON.parse(clientServerStore);

            if(requestCloseOfErrorDiv) {
                requestCloseOfErrorDiv = false;
                if(!dictStore['error-message-div'].includes('hidden')) {
                    dictStore['error-message-div'] += ' hidden';
                }
                return JSON.stringify(dictStore);
            }else{
                return window.dash_clientside.no_update
            }
        }
    }
});