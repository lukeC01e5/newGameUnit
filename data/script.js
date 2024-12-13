/* filepath: /c:/Users/OEM/Documents/GitHub/baseStation/data/script.js */

document.addEventListener("DOMContentLoaded", function() {
    populateAgeDropdown();
});

/**
 * Populates the Age dropdown with options from 1 to 99.
 */
function populateAgeDropdown() {
    const ageSelect = document.getElementById("age");

    for (let i = 1; i <= 99; i++) {
        const option = document.createElement("option");
        option.value = i;
        option.text = i;
        ageSelect.appendChild(option);
    }
}

/**
 * Optional: Add form validation or interactivity here.
 * For example, you can add real-time validation or dynamic form adjustments.
 */