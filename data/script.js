var profileDetected = false;

document.addEventListener("DOMContentLoaded", function() {
    // Only populate age dropdown if it exists on the page
    if (document.getElementById("age")) {
        populateAgeDropdown();
    }
    updateButtonState();
});

// Existing function to populate age dropdown
function populateAgeDropdown() {
    const ageSelect = document.getElementById("age");

    for (let i = 1; i <= 99; i++) {
        const option = document.createElement("option");
        option.value = i;
        option.text = i;
        ageSelect.appendChild(option);
    }
}

// Function to open other site
function openOtherSite() {
    // Your logic to open the other site
    window.location.href = '/otherSite.html';
}

// Function to update the button state based on profileDetected
function updateButtonState() {
    const button = document.getElementById('openOtherSiteButton');
    if (button) {
        button.disabled = !profileDetected;
    }
}

// Example: Change profileDetected to true after certain condition
// Replace this timeout with your actual logic to detect profile
setTimeout(function() {
    profileDetected = true;
    updateButtonState();
}, 3000);