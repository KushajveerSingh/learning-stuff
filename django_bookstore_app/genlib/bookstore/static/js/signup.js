function validation() {
  var pass1 = document.signupForm.userPassword;
  var pass2 = document.signupForm.userPassword2;
  var name = document.signupForm.userFirst_name;
  var name2 = document.signupForm.userLast_name;
  var email = document.signupForm.userEmail;
  var number = document.signupForm.userPhone;
  var check = document.signupForm.something;
  var cvv = document.signupForm.card_cvv;
  var cardName = document.signupForm.card_name;
  var year = document.signupForm.card_year;
  var month = document.signupForm.card_month;
  var cardNumber = document.signupForm.card_num;
  var check = document.signupForm.something;
  var checkBox = document.getElementById("checkBox");
  if (checkAll(name, name2, email, pass1, pass2, number)) {
    if (passValidation(pass1, pass2, 8, 16)) {
      if (nameValid(name, name2)) {
        if (validateEmail(email)) {
          if (validateTerms(checkBox)) {
            if (checkAllCard(cvv, cardName, year, month, cardNumber)) {
              if(lengthValidation(cardNumber, cvv, 16, 3, name, year, month)){

              }
              else {
                  check.value = "not_working";
                  return false;
              }
            } else {
              check.value = "not_working";
              return false;
            }
          }else {
            check.value = "not_working";
          return false;
      }
        }else {
            check.value = "not_working";
          return false;
      }
      }else {
        check.value = "not_working";
      return false;
  }
    }else {
        check.value = "not_working";
      return false;
  }
  }
  else {
        check.value = "not_working";
      return false;
  }
}

function checkAll(name, name2, email, pass1, pass2, number) {
  var pass1_len = pass1.value.length;
  var pass2_len = pass2.value.length;
  if (pass1_len == 0) {
    pass1.classList.add("border-danger");
  }
  if (pass2_len == 0) {
    pass2.classList.add("border-danger");
  }
  if (name.value.length == 0) {
    name.classList.add("border-danger");
  }
  if (name2.value.length == 0) {
    name2.classList.add("border-danger");
  }
  if (number.value.length == 0) {
    number.classList.add("border-danger");
  }
  if (email.value.length == 0) {
    email.classList.add("border-danger");
  }
  if (email.value.length == 0 || name.value.length == 0 || name2.value.length == 0 || number.value.length == 0 || pass2_len == 0 || pass1_len == 0) {
    alert("Fill out all fields");
  }
  if (email.value.length != 0 || name.value.length != 0 || name2.value.length != 0 || number.value.length != 0 || pass2_len != 0 || pass1_len != 0) {
    return true;
  }
}

function passValidation(pass1, pass2, mx, my) {
  var pass1_len = pass1.value.length;
  var pass2_len = pass2.value.length;
  if (pass1.value != pass2.value) {
    alert("Password do not match");
    pass2.focus();
    pass1.classList.add("border-danger");
    pass2.classList.add("border-danger");
    return false;
  }
  if (pass1_len >= my || pass1_len < mx || pass2_len >= my || pass2_len < mx) {
    pass1.classList.add("border-danger");
    pass2.classList.add("border-danger");
    alert("Password should be between " + mx + " to " + my);
    pass1.focus();
    return false;
  } else {
    pass1.classList.remove("border-danger");
    pass2.classList.remove("border-danger");
  }
  return true;
}

function nameValid(name, name2) {
  var letters = /^[A-Za-z]+$/;
  if (name.value.match(letters)) {
    name.classList.remove("border-danger");
    return true;
  } else if (name2.value.match(letters)) {
    name2.classList.remove("border-danger");
    return true;
  } else {
    name.classList.add("border-danger");
    name2.classList.add("border-danger");
    alert('Name can not have numbers or be empty');
    name.focus();
    return false;
  }
}

function validateEmail(email) {
  var mailformat = /^\w+([\.-]?\w+)*@\w+([\.-]?\w+)*(\.\w{2,3})+$/;
  if (email.value.match(mailformat)) {
    email.classList.remove("border-danger");
    return true;
  } else {
    alert("You have entered an invalid email address!");
    email.classList.add("border-danger");
    email.focus();
    return false;
  }
}

function validateTerms(checkBox) {
  if (checkBox.checked == true) {
    checkBox.classList.remove("border-danger");
    return true;
  } else {
    alert("You have not agreed to the terms");
    checkBox.classList.add("border-danger");
    checkBox.focus();
    return false;
  }
}



function checkAllCard(cvv, cardName, year, month, cardNumber) {
  var cvv_len = cvv.value.length;
  var year_len = year.value.length;


    if ((cardName.value.length == 0 || month.value == "Select" || cardNumber.value.length == 0 || year.value == "Select" || cvv_len == 0) && (cardName.value.length != 0 || month.value != "Select" || cardNumber.value.length != 0 || year.value != "Select" || cvv_len != 0)) {
        cardName.classList.add("border-danger");
        month.classList.add("border-danger");
        year.classList.add("border-danger");
        cvv.classList.add("border-danger");
        cardNumber.classList.add("border-danger");

        alert("Fill out all fields");
    }
    if ((cardName.value.length != 0 && month.value != "Select" && cardNumber.value.length != 0 && year.value != "Select" && cvv_len != 0) || (cardName.value.length == 0 && month.value == "Select" && cardNumber.value.length == 0 && year.value == "Select" && cvv_len == 0)) {
        cardName.classList.remove("border-danger");
        month.classList.remove("border-danger");
        year.classList.remove("border-danger");
        cvv.classList.remove("border-danger");
        cardNumber.classList.remove("border-danger");
        return true;
    }
else {
    return true;
}

}

function lengthValidation(cardNumber, cvv, nLen, cLen, cardName, year, month) {
  var cardNumber_len = cardNumber.value.length;
  var cvv_len = cvv.value.length;
  if ((cardName.value.length != 0 && month.value != "Select" && cardNumber.value.length != 0 && year.value != "Select" && cvv_len != 0)) {
    if (cardNumber_len != nLen || cvv_len != cLen) {
      cardNumber.classList.add("border-danger");
      cvv.classList.add("border-danger");
      alert("Incorrect length of Card cardNumber(16) or CVV(3)");
      return false;
    } else {
      cardNumber.classList.remove("border-danger");
      cvv.classList.remove("border-danger");
    }
  }
  return true;
}
